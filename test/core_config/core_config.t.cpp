#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include <i01_core/Config.hpp>

TEST(core_config, core_config_test)
{
    using i01::core::Config;
    using i01::core::ConfigListener;

    class FeedDingus : public ConfigListener {
        public:
            FeedDingus() : m_dingus_name(), m_dingus_size(0), m_dingus_prev_size(0), m_dingus_type() {
                ConfigListener::subscribe();
            }
            virtual ~FeedDingus() {
                ConfigListener::unsubscribe();
            }
            int size() const { return m_dingus_size; }
            int prev_size() const { return m_dingus_prev_size; }
            std::string name() const { return m_dingus_name; }
            std::string type() const { return m_dingus_type; }

            void on_config_update(const Config::storage_type&, const Config::storage_type& cfg) noexcept override final {
                auto name_ = cfg.get<std::string>("dingus.name");
                auto size_ = cfg.get<int>("dingus.size");
                auto prev_size_ = cfg.get<int>("dingus.prev_size");
                auto type_ = cfg.get<std::string>("dingus.type");
                if (name_)
                    m_dingus_name = *name_;
                if (size_)
                    m_dingus_size = *size_;
                if (prev_size_)
                    m_dingus_prev_size = *prev_size_;
                if (type_)
                    m_dingus_type = *type_;
                for (const auto& kv : cfg) {
                    std::cout << "cfg: " << kv.first << " = " << kv.second << std::endl;
                }
            }
        private:
            std::string m_dingus_name;
            int m_dingus_size;
            int m_dingus_prev_size;
            std::string m_dingus_type;
    };

    FeedDingus feed1;
    Config::instance().load_environ();

    boost::program_options::variables_map vm;
    boost::program_options::variable_value vv(std::string("feed"), false);
    vm.emplace(std::make_pair("dingus.type", vv));
    Config::instance().load_variables_map(vm);

    Config::instance().load_lua_file(STRINGIFY(I01_DATA) "/core_config_test.lua");

    EXPECT_EQ(feed1.name(), "my example feed");
    EXPECT_EQ(feed1.size(), 42);
    EXPECT_EQ(feed1.prev_size(), 0);
    EXPECT_EQ(feed1.type(), "feed");
    EXPECT_TRUE(!(Config::instance().get_shared_state()->get<std::string>("nonexistent.bogus")));
    EXPECT_TRUE(!!(Config::instance().get_shared_state()->get<int>("dingus.size")));
    EXPECT_TRUE(!!(Config::instance().get_shared_state()->get<std::string>("dingus.type")));
    Config::instance().load_lua_file(STRINGIFY(I01_DATA) "/core_config_test.lua");
    EXPECT_EQ(feed1.prev_size(), 42);
    EXPECT_TRUE(!!(Config::instance().get_shared_state()->get<int>("dingus.prev_size")));
}
