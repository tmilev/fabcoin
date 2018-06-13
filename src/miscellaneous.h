#ifndef MISCELLANEOUS_header
#define MISCELLANEOUS_header
#include <string>
#include <sstream>
#include <iomanip>

class Miscellaneous
{
public:
  static std::string toStringHex(const std::string& other);
  static std::string toStringShorten(const std::string& input, int numCharactersToRetain);
  template <typename enumerableCollection>
  static std::string toStringVector(const enumerableCollection& input) {
      std::stringstream out;
      out << "(";
      for (unsigned i = 0; i < input.size(); i ++) {
        out << input[i];
        if (i != input.size() - 1) {
            out << ", ";
        }
      }
      out << ")";
      return out.str();
  }
  template <typename enumerableCollection>
  static std::string toStringVectorHex(const enumerableCollection& input) {
      std::stringstream out;
      for (unsigned i = 0; i < input.size(); i ++) {
        out  << std::setfill('0') << std::setw(2) << (int) ((unsigned char) input[i]);
      }
      return out.str();
  }
};


#endif // MISCELLANEOUS_header

