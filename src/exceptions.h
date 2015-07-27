#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

using namespace std;

namespace rambrain
{

class rambrainException : exception
{

public:
    virtual ~rambrainException() {}
};


class incompleteSetupException : rambrainException
{

public:
    incompleteSetupException ( const string &details ) : details ( details ) {}
    virtual ~incompleteSetupException() {}

    inline virtual const char *what() const throw() {
        return ( string ( "Incomplete setup of objects: " ) + details ).c_str();
    }

private:
    string details;

};


class memoryException : rambrainException
{

public:
    memoryException ( const string &details ) : details ( details ) {}
    virtual ~memoryException() {}

    inline virtual const char *what() const throw() {
        return ( string ( "Memory exception: " ) + details ).c_str();
    }

private:
    string details;

};


class unexpectedStateException : rambrainException
{

public:
    unexpectedStateException ( const string &details ) : details ( details ) {}
    virtual ~unexpectedStateException() {}

    inline virtual const char *what() const throw() {
        return ( string ( "Unexpected state detected: " ) + details ).c_str();
    }

private:
    string details;

};


class unfinishedCodeException : rambrainException
{

public:
    unfinishedCodeException ( const string &details ) : details ( details ) {}
    virtual ~unfinishedCodeException() {}

    inline virtual const char *what() const throw() {
        return ( string ( "Unfinished code section encountered: " ) + details ).c_str();
    }

private:
    string details;

};

}

#endif // EXCEPTIONS_H
