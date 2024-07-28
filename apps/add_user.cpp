#include "UsersManager.hpp"



int main() {
    std::string database_address{"localhost"};
    std::string pg_user{"postgres"};
    std::string pg_password{"root"};
    std::string database_name{"screen-viewer"};

    UsersManager manager{database_address, pg_user, pg_password, database_name};

    manager.addUser("some_user@gmail.com", "superStrongPassword");
}