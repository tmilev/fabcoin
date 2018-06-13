#ifndef LOGGING_H_header
#define LOGGING_H_header
#include <fstream>
#include <iostream>

class LoggerSession
{
public:
    static std::string consoleColorRed() {
        return "\e[91m";
    }
    static std::string consoleColorYellow() {
        return "\e[93m";
    }
    static std::string consoleColorNormal() {
        return "\e[39m";
    }
    static std::string consoleColorBlue() {
        return "\e[94m";
    }
    static std::string consoleColorGreen() {
        return  "\e[92m";
    }
    std::fstream theFile;
    std::string descriptionPrependToLogs;
    bool flagIncludeExtraDescriptionInNextLogMessage;
    bool flagDeallocated;
    bool flagHasColor;
    enum logModifiers{ endL, colorBlue, colorRed, colorYellow, colorGreen, colorNormal};
    friend LoggerSession& operator << (LoggerSession& inputLogger, logModifiers other) {
        if (other == LoggerSession::endL) {
            std::cout << std::endl;
            inputLogger.theFile << "\n";
            inputLogger.flagIncludeExtraDescriptionInNextLogMessage = true;
            if (inputLogger.flagHasColor) {
                std::cout << LoggerSession::consoleColorNormal();
                inputLogger.flagHasColor = false;
            }
        }
        switch (other) {
        case LoggerSession::colorBlue:
            std::cout << LoggerSession::consoleColorBlue();
            inputLogger.flagHasColor = true;
            break;
        case LoggerSession::colorGreen:
            std::cout << LoggerSession::consoleColorGreen();
            inputLogger.flagHasColor = true;
            break;
        case LoggerSession::colorRed:
            std::cout << LoggerSession::consoleColorRed();
            inputLogger.flagHasColor = true;
            break;
        case LoggerSession::colorYellow:
            std::cout << LoggerSession::consoleColorYellow();
            inputLogger.flagHasColor = true;
            break;
        case LoggerSession::colorNormal:
            std::cout << LoggerSession::consoleColorNormal();
            inputLogger.flagHasColor = false;
            break;
        default:
            break;
        }
        return inputLogger;
    }
    template<typename any>
    friend LoggerSession& operator << (LoggerSession& inputLogger, const any& other) {
        if (inputLogger.flagDeallocated) {
            std::cout << other << std::endl;
            return inputLogger;
        }
        if (inputLogger.flagIncludeExtraDescriptionInNextLogMessage) {
            inputLogger.flagIncludeExtraDescriptionInNextLogMessage = false;
            std::cout << inputLogger.descriptionPrependToLogs;
        }
        inputLogger.theFile << other;
        inputLogger.theFile.flush();
        std::cout << other;
        return inputLogger;
    }
    LoggerSession(const std::string& pathname, const std::string& inputDescriptionPrependToLogs) {
        this->theFile.open(pathname, std::fstream::out | std::fstream::trunc);
        this->flagIncludeExtraDescriptionInNextLogMessage = true;
        this->descriptionPrependToLogs = inputDescriptionPrependToLogs;
        this->flagDeallocated = false;
        this->flagHasColor = false;
    }
    ~LoggerSession() {
        this->theFile.close();
        this->flagDeallocated = true;
    }
};

#endif // LOGGING_H

