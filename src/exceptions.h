#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

using namespace std;

class membrainException : exception
{

public:
    virtual ~membrainException() {}

};


class incompleteSetupException : membrainException
{

public:
    incompleteSetupException ( const string& details ) : details ( details ) {}
    virtual ~incompleteSetupException() {}

    inline virtual const char* what() const throw() {
        return ( string ( "Incomplete setup of objects: " ) + details ).c_str();
    }

private:
    string details;

};


class memoryException : membrainException
{

public:
    memoryException ( const string& details ) : details ( details ) {}
    virtual ~memoryException() {}

    inline virtual const char* what() const throw() {
        return ( string ( "Memory exception: " ) + details ).c_str();
    }

private:
    string details;

};


class unexpectedStateException : membrainException
{

public:
    unexpectedStateException ( const string& details ) : details ( details ) {}
    virtual ~unexpectedStateException() {}

    inline virtual const char* what() const throw() {
        return ( string ( "Unexpected state detected: " ) + details ).c_str();
    }

private:
    string details;

};

#endif // EXCEPTIONS_H

