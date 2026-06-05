/*
ABCU Program
Description:This program is designed to read course information from a file, load it into a data structure,
            and then allow users to search for and print courses by various criteria.
By:Alisha Brayboy
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <time.h>
#include <sstream> 
#include <ranges>
#include <cctype>
#include <memory>

using namespace std;

/*
GLOBAL DEFINITIONS
*/

//Define a structure to hold course information
struct courses {
    string courseNumber;
    string courseName;
    vector <string> preReq;
};

// Node using unique_ptr children
struct Node {
    courses course;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    Node(const courses& c) : course(c), left(nullptr), right(nullptr) {}
};

// Simplified BST owning nodes via unique_ptr
class BinarySearchTree {
public:
    std::unique_ptr<Node> root;
    BinarySearchTree() = default;

    void insert(const courses& c) {
        if (!root) { root = std::make_unique<Node>(c); return; }
        Node* cur = root.get();
        while (true) {
            if (c.courseNumber < cur->course.courseNumber) {
                if (cur->left) cur = cur->left.get();
                else { cur->left = std::make_unique<Node>(c); break; }
            } else {
                if (cur->right) cur = cur->right.get();
                else { cur->right = std::make_unique<Node>(c); break; }
            }
        }
    }

    void printInOrder(const Node* node) const {
        if (!node) return;
        printInOrder(node->left.get());
        std::cout << node->course.courseNumber << " , " << node->course.courseName << '\n';
        printInOrder(node->right.get());
    }
    void printAll() const { printInOrder(root.get()); }

    // Search by courseNumber and print prerequisites
    void Search(const string& courseNumber) const {
        const Node* current = root.get();
        if (current == nullptr) {
            cout << "No courses loaded." << endl;
            return;
        }

        while (current != nullptr) {
            if (current->course.courseNumber == courseNumber) {
                cout << current->course.courseNumber << " , " << current->course.courseName << endl;
                cout << "Prerequisites: ";
                if (current->course.preReq.empty()) {
                    cout << "None" << endl;
                } else {
                    for (size_t i = 0; i < current->course.preReq.size(); ++i) {
                        cout << current->course.preReq[i];
                        if (i + 1 < current->course.preReq.size()) cout << ", ";
                    }
                    cout << endl;
                }
                return;
            }
            else if (courseNumber < current->course.courseNumber) {
                current = current->left.get();
            }
            else {
                current = current->right.get();
            }
        }
        cout << "Course not found." << endl;
    }
};


/*
END GLOBAL DEFINITIONS
*/

// Method to read course information from a file and load into data structure
// safer sortLoad: resets stream to start, builds a 'courses' and inserts
void sortLoad(std::ifstream& courseFile, BinarySearchTree* bst) {
    if (!courseFile.is_open()) { std::cout << "Error Opening File.\n"; return; }
    courseFile.clear();
    courseFile.seekg(0);
    std::string line;
    while (std::getline(courseFile, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        courses c;
        std::string token;
        if (!std::getline(ss, token, ',')) continue;
        c.courseNumber = token; // normalization performed below
        if (!std::getline(ss, token, ',')) continue;
        c.courseName = token;
        while (std::getline(ss, token, ',')) {
            // optionally trim token here
            c.preReq.push_back(token);
        }
        // normalize course number and prereqs to uppercase for consistent search
        for (char &ch : c.courseNumber) ch = toupper(static_cast<unsigned char>(ch));
        for (auto &p : c.preReq) for (char &ch : p) ch = toupper(static_cast<unsigned char>(ch));

        bst->insert(c);
    }
}

// Helper function to convert a string to uppercase
string toUpperString(const string& str) {
    string result = str;
    for (char& c : result) {
        c = toupper(static_cast<unsigned char>(c));
    }
    return result;
}

//One and Only Main Method
int main() {

    //Declare variables
    ifstream courseFile("CS 300 ABCU_Advising_Program_Input.csv");
    int userChoice = 0;
    string courseNumber;
    
    // Define a binary search tree to hold all courses (automatic storage)
    BinarySearchTree bst;

    cout << "Welcome to the ABCU Course Planner!" << endl;

        //Loop through Menu Choices
        while(userChoice != 9) {
            cout << endl;
            cout << "Menu" << endl;
            cout << "1.Load Courses" << endl;
            cout << "2.Display Courses" << endl;
            cout << "3.Find Prerequisites" << endl;
            cout << "9.Exit" << endl << endl;

            cout << "Enter choice: ";
            cin >> userChoice;
            cout << endl << endl;

            // Validate user input
            if (userChoice < 1 || userChoice > 3 && userChoice != 9) {
                cout << userChoice << " is not a valid option." << endl << endl;
                continue; // Skip to the next iteration of the loop
            }

            switch (userChoice) {

            case 1:
                sortLoad(courseFile, &bst);
                    break;

            case 2:
                cout << "Here is a sample Schedule: " << endl << endl;
                bst.printAll();
                    break;

            case 3:
                cout << "Enter course number to find Prerequisites: ";
                cin >> courseNumber;
                cout << endl;
                bst.Search(toUpperString(courseNumber));
                    break;

            case 9:
                cout << "Thank you for using the Course Planner!";
                break;
            }
    
        }

    return 0;
}