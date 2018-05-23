#include <string>
#include <ctime>
#include <cstdlib>
#include "StringUtil.h"

namespace ceagle {

namespace util {

int randomBetween(int min, int max) {
    srand(time(NULL));
    int randNum = rand()%(max-min + 1) + min;
    return randNum;
}

int useCPU() {
    //clock_t start, end;
    //double runTime;
    //start = clock();
    int i, num = 1, primes = 0;
    int num_max = 500000 + randomBetween(0, 500000);

    while (num <= num_max) {
        i = 2;
        while (i <= num) {
            if (num % i == 0) {
                break;
            }
            i++;
        }
        if (i == num) {
            primes++;
            //std::cout << "Prime: " << num << std::endl;
        }

        num++;
    }

    //end = clock();
    //runTime = (end - start) / (double) CLOCKS_PER_SEC;
    //std::cout << "This machine calculated all " << primes << " under " << num_max << " in " << runTime << " seconds." << std::endl;

    return primes;
}

// https://tfetimes.com/c-count-occurrences-of-a-substring/
// returns count of non-overlapping occurrences of 'sub' in 'str'
int countSubstring(const std::string& str, const std::string& sub) {
    if (sub.length() == 0) return 0;
    int count = 0;
    for (size_t offset = str.find(sub); offset != std::string::npos;
        offset = str.find(sub, offset + sub.length())) {
        ++count;
    }
    return count;
}

// replace <from> to <to> in <str>
// http://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

string getGraphmlStr(const string& ele, const string& content, const vector<string> properties) {
    string ret = "";
    ret = ret + "<" + ele;

    if (!properties.empty()) {
        for (int i = 0; i < properties.size() - 1; ++ i) {
            string propKey = properties[i];
            string propVal = properties[++ i];
            string buffer = "";
            for (unsigned pos = 0; pos != propVal.size(); ++ pos) {
                switch(propVal[pos]) {
                    case '&':
                        buffer = buffer + "&amp;";
                        break;
                    case '%':
                        buffer = buffer + "&#37;";
                        break;
                    case '<':
                        buffer = buffer + "&lt;";
                        break;
                    case '>':
                        buffer = buffer + "&gt;";
                        break;
                    default:
                        buffer = buffer + propVal[pos];
                        break;
                }
            }
            propVal = buffer;
            ret = ret + " " + propKey + "=\"" + propVal + "\"";
        }
    }

    if (content == "") {
        ret = ret + "/>";
    } else {
        if (content[0] == '<') {
            ret = ret + ">\n" + content + "\n</" + ele + ">";
        } else {
            string buffer = "";
            for (unsigned pos = 0; pos != content.size(); ++ pos) {
                switch(content[pos]) {
                    case '&':
                        buffer = buffer + "&amp;";
                        break;
                    case '%':
                        buffer = buffer + "&#37;";
                        break;
                    case '<':
                        buffer = buffer + "&lt;";
                        break;
                    case '>':
                        buffer = buffer + "&gt;";
                        break;
                    default:
                        buffer = buffer + content[pos];
                        break;
                }
            }
            ret = ret + ">" + buffer + "</" + ele + ">";
        }
    }

    return ret;
}
}

}
