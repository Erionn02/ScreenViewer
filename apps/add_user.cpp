#include "UsersManager.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " email password" << std::endl;
        return 1;
    }

    std::string database_address{"localhost"};
    std::string pg_user{"postgres"};
    std::string pg_password{"root"};
    std::string database_name{"screen-viewer"};

    UsersManager manager{database_address, pg_user, pg_password, database_name};

    manager.addUser(argv[1], argv[2]);
}