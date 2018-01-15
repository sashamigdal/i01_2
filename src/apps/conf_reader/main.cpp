// Simple app to print packet counts for UDP addresses

#include <arpa/inet.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <i01_core/Application.hpp>
#include <i01_net/Pcap.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Config.hpp>


class ConfReaderApp : public i01::core::Application, public i01::core::ConfigListener
{
public:

    using Application::Application;
    ConfReaderApp(int argc, const char *argv[]);
    ~ConfReaderApp();
    bool init(int argc, const char *argv[] ) override final;
    virtual int run() override final;
    void on_config_update(const i01::core::Config::storage_type &, const i01::core::Config::storage_type &cfg) noexcept override final;

};

ConfReaderApp::ConfReaderApp(int argc, const char *argv[]) : Application()
{
    i01::core::ConfigListener::subscribe();

    init(argc,argv);
}

ConfReaderApp::~ConfReaderApp()
{
    i01::core::ConfigListener::unsubscribe();
}

void ConfReaderApp::on_config_update(const i01::core::Config::storage_type &, const i01::core::Config::storage_type &cfg) noexcept
{
    for (const auto &kv : cfg) {
        std::cout << "cfg " << cfg.version() << ": " << kv.first << " = " << kv.second << std::endl;
    }
}

bool ConfReaderApp::init(int argc, const char *argv[])
{
    Application::init(argc,argv);
    return true;
}

int ConfReaderApp::run()
{
    return 0;
}



int main(int argc, const char *argv[])
{
    ConfReaderApp app(argc, argv);
    return app.run();
}
