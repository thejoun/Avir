#include <iostream>
#include <string>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <sys/wait.h>

#include "scan.h"

using namespace std;
using namespace boost::filesystem;

void print_usage()
{
    cout << "Usage: avir [type] [option]" << endl;
    cout << " Types:" << endl;
    cout << "  -f  <path>:  scan a single file" << endl;
    cout << "  -dl <path>:  scan a directory linearly" << endl;
    cout << "  -dr <path>:  scan a directory recursively" << endl;
    cout << " Options:" << endl;
    cout << "  -o  <file>:  save output to file" << endl;
}

void find_file(vector<path>& filePaths, path& scanPath)
{
    if(is_regular_file(scanPath)){
        filePaths.push_back(scanPath);
    }
}

void find_files_linear(vector<path>& filePaths, path& scanPath)
{
    for(directory_iterator iterator(scanPath); iterator != directory_iterator(); ++iterator)
    {
        path path = iterator -> path();
        if (is_regular_file(path))
        {
            filePaths.push_back(path);
        }
    }
}

void find_files_recursive(vector<path>& filePaths, path& scanPath)
{
    for(recursive_directory_iterator iterator(scanPath); iterator != recursive_directory_iterator(); ++iterator)
    {
        path path = iterator -> path();
        if (is_regular_file(path))
        {
            filePaths.push_back(path);
        }
    }
}

int main(int argc, char *argv[])
{
    // determine arguments

    if(argc == 1){
        print_usage();
        return 0;
    }

    if(argc % 2 == 0){
        cout << "Wrong usage." << endl;
        print_usage();
        return 0;
    }

    // determine type and scan path

    string arg1 = argv[1];
    string arg2 = argv[2];

    Scan::scan_type scanType;
    path scanPath;

    if(arg1 == "-f"){
        if(!is_regular_file(arg2)){
            cout << "That's not a file." << endl;
            return 0;
        }
        scanType = Scan::file_scan;
    }
    else if(arg1 == "-dl"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 0;
        }
        scanType = Scan::dir_linear_scan;
    }
    else if(arg1 == "-dr"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 0;
        }
        scanType = Scan::dir_recursive_scan;
    }
    else{
        cout << "Wrong type." << endl;
        print_usage();
        return 0;
    }

    scanPath = canonical(arg2);

    // determine options

    path outputPath;

    if(argc >= 5){
        string arg3 = argv[3];
        string arg4 = argv[4];

        if(arg3 == "-o"){
            std::ofstream file {arg4};
            if(access(arg4.c_str(), F_OK) != -1){
                outputPath = canonical(arg4);
            }
        }
        else{
            cout << "Wrong option." << endl;
            print_usage();
            return 0;
        }
    }

    // search for files

    std::vector<path> filePaths;

    switch(scanType){
        case Scan::file_scan:
            find_file(filePaths, scanPath);
            break;
        case Scan::dir_linear_scan:
            cout << "Searching for files..." << endl;
            find_files_linear(filePaths, scanPath);
            break;
        case Scan::dir_recursive_scan:
            cout << "Searching for files..." << endl;
            find_files_recursive(filePaths, scanPath);
            break;
    }

    if(scanType == Scan::dir_linear_scan || scanType == Scan::dir_recursive_scan){
        if(filePaths.empty()){
            cout << "Found no files at " << scanPath << endl;
        }

        cout << "Found " << filePaths.size() << " files at " << scanPath << endl;

        string yn;
        cout << "Do you want to continue? [Y/n] ";
        cin >> yn;
        if(yn != "y" && yn != "Y"){
            return 0;
        }
    }

    //pid_t forkPid = fork();

    switch(fork()){
        case -1: {
            printf("Fork error! Scan aborted.");
            return -1;
        }
        case 0: {
            Scan::scan scan;
            scan.scanType = scanType;
            scan.scanPath = scanPath;
            scan.filePaths = filePaths;
            scan.outputPath = outputPath;
            scan.begin();
            return 0;
        }
        default: {
            cout << "Scan started." << endl;
        }
    }
}