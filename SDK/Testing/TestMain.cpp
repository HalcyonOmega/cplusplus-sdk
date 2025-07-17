#include <iostream>

void ShowMenu() {
    std::cout << "\n=== MCP Transport Test ===" << std::endl;
    std::cout << "1. Test STDIO Transport" << std::endl;
    std::cout << "2. Test HTTP Transport" << std::endl;
    std::cout << "3. Exit" << std::endl;
    std::cout << "Choice: ";
}

int main() {
    std::cout << "MCP SDK Transport Interactive Test" << std::endl;

    int choice;
    while (true) {
        ShowMenu();
        std::cin >> choice;

        switch (choice) {
            case 1: std::cout << "Stdio Testing..." << std::endl; break;
            case 2: std::cout << "HTTP Testing..." << std::endl; break;
            case 3: std::cout << "Exiting..." << std::endl; return 0;
            default: std::cout << "Invalid choice!" << std::endl; break;
        }
    }

    return 0;
}
