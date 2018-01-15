#include <i01_core/Application.hpp>
#include <i01_core/Config.hpp>


class PyOrderLogApp : public i01::core::Application
{
public:

    using Application::Application;
    PyOrderLogApp(int argc, const char *argv[]);
    ~PyOrderLogApp();
    bool init(int argc, const char *argv[] ) override final;
    virtual int run() override final;

};

PyOrderLogApp::PyOrderLogApp(int argc, const char *argv[]) : Application()
{

    init(argc,argv);
}

PyOrderLogApp::~PyOrderLogApp()
{
}

bool PyOrderLogApp::init(int argc, const char *argv[])
{
    Application::init(argc,argv);
    return true;
}

int PyOrderLogApp::run()
{
    return 0;
}



int main(int argc, const char *argv[])
{
    PyOrderLogApp app(argc, argv);
    return app.run();
}
