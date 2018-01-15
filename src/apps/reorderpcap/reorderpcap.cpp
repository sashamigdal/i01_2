#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <iostream>
#include <sstream>
#include <queue>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <i01_core/Pcap.hpp>

#include "reorderpcap.hpp"

namespace bfs = boost::filesystem;
namespace po = boost::program_options;

using namespace i01::apps::reorderpcap;

static std::string s_input_file = "";
static std::string s_output_file = "";
static int64_t s_batch_size = 0;
static int s_num_passes_remaining = 0;
static bool s_no_fallback_unchunked = false;

int pass0(bfs::path& p, bfs::path& op) {
    if (bfs::equivalent(p, op) || p == op) {
        std::cerr << "ERROR: Output location must differ from input location: " << p << std::endl;
        return EXIT_FAILURE;
    }
    if (!bfs::exists(p)) {
        std::cerr << "ERROR: " << p << " does not exist, skipping." << std::endl;
        return EXIT_FAILURE;
    }
    if (bfs::exists(op)) {
        std::cerr << "ERROR: " << op << " already exists." << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int64_t pass1(bfs::path& p, bfs::path& op, int64_t bs = 0) {
    char errbuf[PCAP_ERRBUF_SIZE];
    ::memset(errbuf, 0, sizeof(errbuf));

    i01::core::pcap::FileReader fr(p.c_str(), errbuf);
    std::priority_queue<Record, std::vector<Record>, std::greater<Record>> recs;
    struct pcap_pkthdr *pkthdr = nullptr;
    const u_char *pktdata = nullptr;
    std::int64_t seqnum = 0;
    int64_t n = 0; // output seqnum
    auto * pc = reinterpret_cast<pcap_t*>(fr.get_pcap_t_ptr());
    if (!pc) {
        std::cerr << "ERROR: When opening " << p << " for reading: " << errbuf << std::endl;
        return -1;
    }
    bfs::path op_pass(op.string() + "." + std::to_string(bs));
    if (bfs::exists(op_pass)) {
        std::cerr << "ERROR: " << op << " already exists." << std::endl;
        return -1;
    }
    auto* opc = ::pcap_dump_open(pc, op_pass.c_str());
    if (!opc) {
        std::cerr << "ERROR: When opening " << op_pass << " for writing: " << pc->errbuf << std::endl;
        return -1;
    }
    FILE * ofp = ::pcap_dump_file(opc);
    //fpos64_t istart;
    //::fgetpos64(pc->rfile, &istart);
    if (bs <= 0)
        std::cout << "INFO: Reading " << p;
    else
        std::cout << "INFO: Reading(.) + writing(:) " << p << " + " << op_pass;
    std::cout.flush();
    int pne = fr.next_ex(&pkthdr, &pktdata);
    i01::core::Timestamp last_ts{0,0};
    for ( ; pne == 1; pne = fr.next_ex(&pkthdr, &pktdata)) {
        try {
            Record rec;
            rec.ts = i01::core::Timestamp{ pkthdr->ts.tv_sec, pkthdr->ts.tv_usec };
            rec.seqnum = seqnum;
            rec.buf = ::malloc(pkthdr->caplen + sizeof(pcap_pkthdr) + 1);
            if (rec.buf == NULL) {
                std::cerr << "ERROR: Ran out of memory while ingesting packet number " << seqnum << "." << std::endl;
                ::pcap_dump_close(opc);
                bfs::remove(op_pass);
                return -1;
            }
            ::memcpy(&rec.hdr, pkthdr, sizeof(*pkthdr));
            ::memcpy(rec.buf, pktdata, pkthdr->caplen);
            recs.emplace(std::move(rec));
            ++seqnum;
            if (seqnum % 1000000 == 0) {
                std::cout << ".";
                std::cout.flush();
            }
            if ( (bs > 0) 
              && (recs.size() >= static_cast<size_t>(bs))
              && (::ferror(ofp) == 0)) {
                const Record& r(recs.top());
                if (last_ts > r.ts) {
                    // Switch to unchunked mode since chunk size is too small.
                    std::cerr << std::endl;
                    std::cerr << "WARNING: Chunked pass failed "
                              << "since chunk size is too small at " << seqnum << " (" << bs << ")" << std::endl;
                    ::pcap_dump_close(opc);
                    std::cerr << "WARNING: Deleting chunk size " << bs << " pass file " << op_pass << std::endl;
                    bfs::remove(op_pass);
                    while (!recs.empty()) {
                        ::free(recs.top().buf);
                        recs.pop();
                    }
                    return seqnum;
                } else {
                    ::pcap_dump((u_char*)opc, &r.hdr, (u_char*)r.buf);
                    last_ts = r.ts;
                    ::free(r.buf);
                    recs.pop();
                    ++n;
                    if (n % 1000000 == 0) {
                        std::cout << ":";
                        std::cout.flush();
                    }
                }
            }
        } catch (std::bad_alloc &e) {
            std::cerr << "ERROR: Ran out of memory while processing packet number " << seqnum << "." << std::endl;
            ::pcap_dump_close(opc);
            bfs::remove(op_pass);
            return -1;
        }
    }
    std::cout << " " << seqnum << std::endl;
    if (pne != -2) {
        if (pne == -1) {
            std::cerr << "ERROR: While reading " << p << ": " << pne << " " << fr.geterr() << std::endl;
            ::pcap_dump_close(opc);
            bfs::remove(op_pass);
            return -1;
        } else {
            std::cerr << "ERROR: While reading " << p << ": unknown error." << std::endl;
            ::pcap_dump_close(opc);
            bfs::remove(op_pass);
            return -1;
        }
    } else {
        // We reached the end of the file.
        std::cout << "INFO: Writing " << op;
        std::cout.flush();
        while ((!recs.empty()) && (::ferror(ofp) == 0)) {
            const Record& r(recs.top());
            ::pcap_dump((u_char*)opc, &r.hdr, (u_char*)r.buf);
            ::free(r.buf);
            recs.pop();
            ++n;
            if (n % 1000000 == 0) {
                std::cout << ".";
                std::cout.flush();
            }
        }
        std::cout << " " << n << std::endl;
        ::pcap_dump_flush(opc);
        ::pcap_dump_close(opc);
        try {
            bfs::rename(op_pass, op);
        } catch (bfs::filesystem_error& e) {
            std::cerr << "ERROR: Failed to rename " << op_pass << " to " << op << std::endl;
            //bfs::remove(op_pass);
            return -2;
        }
        std::cout << "SUCCESS: " << op << "." << std::endl;
        return 0;
    }
    // Should never happen:
    return -1;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

    po::options_description desc;
    desc.add_options()
        ("help,h", "Display usage.")
        ("input-file,i"
          , po::value<std::string>()
          , "Input PCAP-NS file.")
        ("output-file,o"
          , po::value<std::string>()
          , "Output PCAP-NS file.")
        ("num-passes,n"
          , po::value<int>()->default_value(1)
          , "Number of passes of chunked sorting to try:  Multiplies initial-pass-chunk-size by 2 each pass, and falls back to unchunked sorting otherwise.  Default 0 = one-pass unchunked sorting.")
        ("initial-pass-chunk-size,s"
          , po::value<int64_t>()->default_value(1000000)
          , "Enable chunked sorting:  The first pass sorts in chunks of this many packets.  If the first pass, N packets are sorted at a time.  If the first num-passes are unsuccessful in sorting the file, the whole file is sorted as one chunk.  This limits the amount of RAM needed to the chunk size in the partially sorted case.")
        ("no-fallback-unchunked"
          , po::bool_switch()->default_value(false)
          , "Do not perform unchunked sorting if chunked sorting failed.  Only valid when using chunked sorting.")
        ;
    po::positional_options_description pdesc;
    pdesc.add("input-file", 1);
    pdesc.add("output-file", 1);
    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pdesc).run(), vm);
        po::notify(vm);
    } catch (std::exception& e) {
        std::cerr << std::endl << e.what() << std::endl
                  << desc << std::endl;
        return EXIT_FAILURE;
    }
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }
    if (!vm.count("input-file")) {
        std::cerr << "ERROR: No input file specified." << std::endl;
        std::cerr << desc << std::endl;
        return EXIT_FAILURE;
    }
    if (!vm.count("output-file")) {
        std::cerr << "ERROR: No output file specified." << std::endl;
        std::cerr << desc << std::endl;
        return EXIT_FAILURE;
    }
    if (vm["initial-pass-chunk-size"].as<int64_t>() < 0) {
        std::cerr << "ERROR: initial-pass-chunk-size must be >= 0." << std::endl;
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    s_input_file = vm["input-file"].as<std::string>();
    s_output_file = vm["output-file"].as<std::string>();
    s_num_passes_remaining = vm["num-passes"].as<int>();
    s_batch_size = vm["initial-pass-chunk-size"].as<int64_t>();
    s_no_fallback_unchunked = vm["no-fallback-unchunked"].as<bool>();
    if (s_num_passes_remaining <= 0 || s_batch_size <= 0) {
        s_num_passes_remaining = 0;
        s_batch_size = 0;
    }
    if (s_batch_size == 0 && s_no_fallback_unchunked) {
        std::cerr << "ERROR: no-fallback-unchunked specified with unchunked sorting." << std::endl;
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }
    bfs::path p(s_input_file);
    bfs::path op(s_output_file);

    if ((ret = pass0(p, op)) != EXIT_SUCCESS)
        return ret;

    int64_t ret_pass1 = -1;
    do {
        if ((s_num_passes_remaining <= 0 || s_batch_size == 0) && s_no_fallback_unchunked) {
            std::cerr << "ERROR: Not falling back on unchunked sorting." << std::endl;
            return EXIT_FAILURE;
        }
        ret_pass1 = pass1(p, op, s_num_passes_remaining > 0 ? s_batch_size : 0);
        if (ret_pass1 == 0)
            return EXIT_SUCCESS;
        else if (ret_pass1 < 0) {
            return EXIT_FAILURE;
        }
        else if (ret_pass1 > 0 && s_num_passes_remaining > 0) {
            s_num_passes_remaining -= 1;
            if (s_num_passes_remaining == 0) {
                s_batch_size = 0;
            } else {
                s_batch_size *= 2;
            }
            continue;
        }
    } while (ret_pass1 > 0);

    return EXIT_FAILURE;
}
