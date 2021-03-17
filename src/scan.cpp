#include <iostream>
#include <chrono>
#include <ctime>

#include <boost/filesystem.hpp>

#include "scan.h"
#include "hash.h"

using namespace std;
using namespace boost::filesystem;

namespace Scan
{
    scan::scan(){

    }

    void scan::begin()
    {
        auto start = std::chrono::system_clock::now();

        //int fileCount = filePaths.size();
        int i = 1;
        for(auto & file : filePaths){
            //cout << i << "/" << fileCount << "\t" << file.string() << endl;
            fileScanResults.push_back(scan_file(file.string()));
            i++;
        }

        int resultCount = fileScanResults.size();
        int unreadable_count = 0;
        int safe_count = 0;
        vector<file_scan_result> unsafeResults;

        for(auto & result : fileScanResults){
            switch(result.state){
                case is_not_readable:
                    unreadable_count++;
                    break;
                case is_safe:
                    safe_count++;
                    break;
                case is_not_safe:
                    unsafeResults.push_back(result);
                    break;
            }
        }

        auto end = std::chrono::system_clock::now();

        time_t start_time = chrono::system_clock::to_time_t(start);
        time_t end_time = chrono::system_clock::to_time_t(end);
        chrono::duration<double> elapsed_seconds = end-start;

        string scanTypeName;
        switch(scanType){
            case file_scan: scanTypeName = "single file scan"; break;
            case dir_linear_scan: scanTypeName = "linear directory scan"; break;
            case dir_recursive_scan: scanTypeName = "recursive directory scan"; break;
        }

        if(outputPath.empty()){
            return;
        }

        //cout << "Output saved to " << outputPath << endl;

        boost::filesystem::ofstream outStream(outputPath);

        outStream << "AVIR SCAN REPORT" << endl;
        outStream << " --- " << endl;
        outStream << "Scan path: \t" << scanPath.string() << endl;
        outStream << "Scan type: \t" << scanTypeName << endl;
        outStream << " --- " << endl;
        outStream << "Start time: \t" << ctime(&start_time);
        outStream << "End time: \t" << ctime(&end_time);
        outStream << "Elapsed: \t" << elapsed_seconds.count() << " seconds" << endl;
        outStream << " --- " << endl;
        outStream << "File count: \t" << resultCount << endl;
        outStream << "  Safe: \t" << safe_count << endl;
        outStream << "  Unsafe: \t" << unsafeResults.size() << endl;
        outStream << "  Unreadable: \t" << unreadable_count << endl;
        outStream << endl;
    }

    string scan::execute(const char* cmd)
    {
        array<char, 128> buffer;
        string result;

        unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

        if(!pipe) throw runtime_error("popen() failed!");

        while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
            result += buffer.data();
        }
        return result;
    }

    file_scan_result scan::scan_file(const path& path)
    {
        file_scan_result result;

        result.path = path;

        result.hash = Hash::md5(path.string());

        if(result.hash.empty()){
            result.state = is_not_readable;
            return result;
        }

        string requestStr = "whois -h hash.cymru.com " + result.hash;
        const char* request = &requestStr[0];

        string response = execute(request);

        if(response.find("NO_DATA") != string::npos){
            result.state = is_safe;
        }
        else{
            result.state = is_not_safe;
        }

        return result;
    }
}