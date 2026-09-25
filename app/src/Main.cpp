#include <app/GUIController.hpp>
#include <app/MainController.hpp>
#include <app/WorkshopApp.hpp>

void WorkshopApp::app_setup() {
    auto main_controller = register_controller<MainController>();
    auto gui_controller = register_controller<GUIController>();
    main_controller->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
    gui_controller->after(main_controller);
}

int main(int argc, char **argv) {
    return std::make_unique<WorkshopApp>()->run(argc, argv);
}
