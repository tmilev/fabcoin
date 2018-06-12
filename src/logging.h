#ifndef LOGGING_H_header
#define LOGGING_H_header
#include <fstream>
#include <iostream>

class LoggerSession
{
public:
  static std::string colorRed;
  static std::string colorYellow;
  static std::string colorGreen;
  static std::string colorBlue;
  static std::string colorNormal;

  std::fstream theFile;
  std::string descriptionPrependToLogs;
  bool flagExtraDescription;
  bool flagDeallocated;
  enum logModifiers{ endL};
  friend LoggerSession& operator << (LoggerSession& inputLogger, logModifiers other) {
    if (other == LoggerSession::endL) {
      std::cout << std::endl;
      inputLogger.theFile << "\n";
    }
    inputLogger.flagExtraDescription = true;
    return inputLogger;
  }
  template<typename any>
  friend LoggerSession& operator << (LoggerSession& inputLogger, const any& other) {
    if (inputLogger.flagDeallocated) {
      std::cout << other << std::endl;
      return inputLogger;
    }
    if (inputLogger.flagExtraDescription) {
      inputLogger.flagExtraDescription = false;
      std::cout << inputLogger.descriptionPrependToLogs;
    }
    inputLogger.theFile << other;
    inputLogger.theFile.flush();
    std::cout << other;
    return inputLogger;
  }
  LoggerSession(const std::string& pathname, const std::string& inputDescriptionPrependToLogs) {
    this->theFile.open(pathname, std::fstream::out | std::fstream::trunc);
    this->flagExtraDescription = true;
    this->descriptionPrependToLogs = inputDescriptionPrependToLogs;
    this->flagDeallocated = false;
  }
  ~LoggerSession() {
    this->theFile.close();
    this->flagDeallocated = true;
  }
};

#endif // LOGGING_H

