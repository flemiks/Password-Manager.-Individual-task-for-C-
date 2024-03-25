#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <functional>
#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
int getch(void) {
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}
#endif
#include "AES.hpp"

using namespace std;

template<typename T>
vector<T> stringToVector(const string& data, function<T(const string&, const string&)> createItem) {
    vector<T> items;
    istringstream stream(data);
    string first, second;

    while (getline(stream, first) && getline(stream, second)) {
        items.push_back(createItem(first, second));
    }

    return items;
}

const string RED = "\033[31m";      
const string GREEN = "\033[32m";    
const string YELLOW = "\033[33m";   
const string BLUE = "\033[34m";    
const string MAGENTA = "\033[35m";  
const string CYAN = "\033[36m";     
const string RESET = "\033[0m";

struct masterPasswordFileName {
    string masterPassword;
    string fileName;
};

struct ResourcePassword {
    string resourceName;
    string password;
};

string getEncryptedDataFromFile(const string& fileName) {
    ifstream file(fileName, ios::binary);
    if (!file.is_open()) return "";
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

void writeDataToFile(const string& fileName, const vector<unsigned char>& data) {
    ofstream file(fileName, ios::binary | ios::trunc);
    if (file.is_open()) file.write(reinterpret_cast<const char*>(data.data()), data.size());
    else cout << RED << "Unable to open file for writing." << RESET << endl;
    cout << endl;
}

string vectorToStringMasterPasswordFileName(const vector<masterPasswordFileName>& items) {
    stringstream stream;
    for (const auto& item : items) {
        stream << item.masterPassword << "\n" << item.fileName << "\n";
    }
    return stream.str();
}

string vectorToStringResourcePassword(const vector<ResourcePassword>& passwords) {
    stringstream ss;
    for (const auto& entry : passwords) {
        ss << entry.resourceName << "\n" << entry.password << "\n";
    }
    return ss.str();
}

string getPasswordWithAsterisk(const string& prompt) {
    cout << YELLOW << prompt << RESET;
    string password;
    char ch;
    while (true) {
        ch = _getch();
        if (ch == '\r' || ch == '\n') {
            cout << endl;
            break;
        }
        else if (ch == '\b' || ch == 127) {
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
            }
        }
        else {
            password.push_back(ch);
            cout << '*';
        }
    }
    return password;
}

string getNewMasterPassword() {
    string firstTry, secondTry;
    do {
        firstTry = getPasswordWithAsterisk("Enter your new Master Password: ");
        secondTry = getPasswordWithAsterisk("Repeat your new Master Password again: ");
        if (firstTry != secondTry) {
            cout << RED << "Both attempts to enter Master Password should match. Please try again." << RESET << endl;
            cout << endl;
        }
    } while (firstTry != secondTry);
    return firstTry;
}

vector<unsigned char> encryptData(unsigned char key[], const string& data) {
    Cipher::Aes<128> aes(key);
    vector<unsigned char> buffer(data.begin(), data.end());

    size_t padLength = 16 - buffer.size() % 16;
    buffer.insert(buffer.end(), padLength, padLength);

    for (size_t i = 0; i < buffer.size(); i += 16) aes.encrypt_block(&buffer[i]);
    return buffer;
}

string decryptData(unsigned char key[], const vector<unsigned char>& encrypted) {
    Cipher::Aes<128> aes(key);
    vector<unsigned char> buffer = encrypted;

    for (size_t i = 0; i < buffer.size(); i += 16) aes.decrypt_block(&buffer[i]);

    size_t padLength = buffer.back();
    buffer.resize(buffer.size() - padLength);

    return string(buffer.begin(), buffer.end());
}

string createNewPasswordListFile(const vector<masterPasswordFileName>& items) {
    string newFileName = "PasswordList_" + to_string(items.size() - 1);
    ofstream newPasswordFile(newFileName);
    if (newPasswordFile.is_open()) {
        cout << GREEN << "New password file " << YELLOW << newFileName << GREEN << " has been created successfully." << RESET << endl;
        newPasswordFile.close();
        return newFileName;
    }
    else {
        cout << RED << "Failed to create new password file " << newFileName << "." << RESET << endl;
        cout << endl;
        return "";
    }
}

string getDecryptedDataFromFile(unsigned char key[], const string& fileName) {
    string encryptedData = getEncryptedDataFromFile(fileName);
    if (encryptedData == "") {
        cout << RED << "Data file " << fileName << " not found." << RESET << endl;
        cout << endl;
        return "";
    }
    vector<unsigned char> encrypted(encryptedData.begin(), encryptedData.end());
    string decryptedData = decryptData(key, encrypted);
    
    return decryptedData;
}

void addPasswordToPasswordList(unsigned char key[], const string& fileName) {
    string decryptedData = getDecryptedDataFromFile(key, fileName);

    auto passwords = stringToVector<ResourcePassword>(decryptedData, [](const string& resourceName, const string& password) {
        return ResourcePassword{ resourceName, password };
        });
    string resourceName, password, preCin;
    cout << endl;
    cout << GREEN << "Add source address and password for example \"example@mail.com\" \"password123\" " << RESET << endl;
    cout << YELLOW << "To exit data entry mode, enter" << RED <<" \"exit\" " << RESET << endl;
    cout << endl;
    do
    { 
        cout << MAGENTA << "Enter resource name: " << RESET;
        cin >> preCin;
        if (preCin == "exit") break;
        resourceName = preCin;
        cout << BLUE << "Enter password: " << RESET;
        cin >> preCin;
        if (preCin == "exit") break;
        password = preCin;
        passwords.push_back({ resourceName, password });
    } while (true);

    string newData = vectorToStringResourcePassword(passwords);
    vector<unsigned char> newEncrypted = encryptData(key, newData);
    writeDataToFile(fileName, newEncrypted);

    cout << GREEN << "Password list has been updated successfully." << RESET << endl;
    cout << endl;
}

void addNewMasterPassword(unsigned char key[], const string& fileName) {
    string decryptedData = getDecryptedDataFromFile(key, fileName);

    auto items = stringToVector<masterPasswordFileName>(decryptedData, [](const string& masterPassword, const string& fileName) {
        return masterPasswordFileName{ masterPassword, fileName };
        });
    string newPassword = getNewMasterPassword();
    items.push_back({ newPassword, "PasswordList_" + to_string(items.size()) });
    string newFile = createNewPasswordListFile(items);
    addPasswordToPasswordList(key, newFile);

    string data = vectorToStringMasterPasswordFileName(items);
    vector<unsigned char> newEncrypted = encryptData(key, data);
    writeDataToFile(fileName, newEncrypted);

    cout << GREEN << "New password list has been saved successfully." << RESET << endl;
    cout << endl;
}


void deletePasswordFromPasswordList(unsigned char key[], const string& fileName, const string& resourceNameToDelete) {
    string decryptedData = getDecryptedDataFromFile(key, fileName);
    if (decryptedData == "") {
        cout << RED << "Password List is empty or does not exist." << RESET << endl;
        cout << endl;
        return;
    }
   
    auto passwords = stringToVector<ResourcePassword>(decryptedData, [](const string& resourceName, const string& password) {
        return ResourcePassword{ resourceName, password };
        });
    auto originalSize = passwords.size();
    passwords.erase(remove_if(passwords.begin(), passwords.end(),
        [&resourceNameToDelete](const ResourcePassword& resource) {
            return resource.resourceName == resourceNameToDelete;
        }),
        passwords.end());
    if (passwords.size() == originalSize) {
        cout << RED << "Resource does not exist. Nothing has been deleted" << RESET << endl;
    }

    string newData = vectorToStringResourcePassword(passwords);
    vector<unsigned char> newEncrypted = encryptData(key, newData);
    writeDataToFile(fileName, newEncrypted);
}

void printPasswordList(unsigned char key[], const string& fileName) {
    string decryptedData = getDecryptedDataFromFile(key, fileName);
    if (decryptedData == "") {
        cout << RED << "Password List is empty or does not exist." << RESET << endl;
        cout << endl;
        return;
    }
    
    auto passwords = stringToVector<ResourcePassword>(decryptedData, [](const string& resourceName, const string& password) {
        return ResourcePassword{ resourceName, password };
        });
    for (const auto& item : passwords) {
        cout << MAGENTA << "Resource Name: " << RESET << item.resourceName << CYAN << ", Password: "<< RESET << item.password  << endl;
    }
    cout << endl;
}

string getFileNameByMasterPassword(unsigned char key[], const string& inputPassword, const string& fileName) {
    string decryptedData = getDecryptedDataFromFile(key, fileName);
    if (decryptedData == "") {
        cout << RED << "Any Master passwords dont exist. Be sure that file mainData.dat in the same directory with exe file " << RESET << endl;
        cout << endl;
        return "";
    }
    
    auto items = stringToVector<masterPasswordFileName>(decryptedData, [](const string& masterPassword, const string& fileName) {
        return masterPasswordFileName{ masterPassword, fileName };
        });
    for (const auto& item : items) {
        if (item.masterPassword == inputPassword) {
            return item.fileName;
        }
    }
    cout << RED << "Master password is incorrect or does not exist." << RESET << endl;
    cout << endl;
    return "";

}

int main() {
    unsigned char key[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    };
    string passwordValidate;
    string fileName;
    string resourceNameToDelete;
    int selection = 0;
    do {
        cout << RED << "Select Menu item:" << RESET << endl;
        cout << GREEN << "1. To Enter Master Password to print existing password list." << endl;
        cout << "2. To Enter Master Password to add new passwords to existing password list." << endl;
        cout << "3. To Enter Master Password to delete passwords from existing password list."  << endl;
        cout << "4. To Create new password list." << RESET << endl;
        cout << YELLOW << "5. Exit." << RESET << endl;
        cout << endl;
        cout <<"--------------------------------------------------------------------------------" << endl;
        cout << GREEN << "Enter Menu number: " << RESET;
        while (!(cin >> selection)) {
            if (cin.eof()) {
                cout << "Entry completed unexpectedly." << endl;
                return 1;
            }
            else if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << RED << "Wrong input. Please enter 1, 2, 3 or 3 to exit" << RESET << endl;
            }
        }
        switch (selection) {
        case 1:
            passwordValidate = getPasswordWithAsterisk("Enter your Master Password: ");
            cout << endl;
            fileName = getFileNameByMasterPassword(key, passwordValidate, "mainData.dat");
            if (fileName != "") {
                printPasswordList(key, fileName);
            }
            break;
        case 2:
            passwordValidate = getPasswordWithAsterisk("Enter your Master Password: ");
            cout << endl;
            fileName = getFileNameByMasterPassword(key, passwordValidate, "mainData.dat");
            if (fileName != "") {
                addPasswordToPasswordList(key, fileName);
            }
            break;
        case 3:
            passwordValidate = getPasswordWithAsterisk("Enter your Master Password: ");
            cout << endl;
            fileName = getFileNameByMasterPassword(key, passwordValidate, "mainData.dat");
            if (fileName != "") {
                printPasswordList(key, fileName);
                cout << YELLOW << "Enter resource to delete: " << RESET;
                cin >> resourceNameToDelete;
                deletePasswordFromPasswordList(key, fileName, resourceNameToDelete);
                printPasswordList(key, fileName);
            }
            break;
        case 4:
            addNewMasterPassword(key,"mainData.dat");
            break;
        case 5:
            cout << "Exiting program." << endl;
            exit(0);
        default:
            cout << RED << "Wrong input. Please enter 1, 2, 3 or 3 to exit" << RESET << endl;
            cout << endl;
            break;
        }
    } while (true);

    return 0;
}