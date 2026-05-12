#include "application.hpp"

extern "C" void app_main() {
	Application::GetInstance().Start();
}
