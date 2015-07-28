/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

using namespace std;

namespace rambrain
{

/**
 * @brief Exception base class for this library
 * @note We provide exceptions for grave errors in this library.
 * They can be caught to prevent crashing of any user code however the memory management system is usually totally broken the moment an exception is thrown.
 * Therefore it is not advised to encapsulate everything by default with try catch blocks but to only do so if something can be really done.
 * Exceptions are NOT used to regulate normal program flow.
 */
class rambrainException : exception
{

public:
    /**
     * @brief Destroy the exception
     */
    virtual ~rambrainException() {}
};


/**
 * @brief Exception class for cases of an incomplete setup
 *
 * Happens for example if no memory manager is in place or something like this. However due to the config and defaults system this should hardly be encountered anymore.
 */
class incompleteSetupException : rambrainException
{

public:
    /**
     * @brief Create a new exception
     * @param details Optionally provided details about the current state of affairs
     */
    incompleteSetupException ( const string &details ) : details ( details ) {}
    /**
     * @brief Destroy the exception
     */
    virtual ~incompleteSetupException() {}

    /**
     * @brief Report what has gone wrong
     * @return The report
     */
    inline virtual const char *what() const throw() {
        return ( string ( "Incomplete setup of objects: " ) + details ).c_str();
    }

private:
    string details;

};


/**
 * @brief Exception for errors with the memory
 *
 * Happens for example when swapping did unexpectedly not work or perhaps the swap is fully and not designed to be overfilled
 */
class memoryException : rambrainException
{

public:
    /**
     * @brief Create a new exception
     * @param details Optionally provided details about the current state of affairs
     */
    memoryException ( const string &details ) : details ( details ) {}
    /**
     * @brief Destroy the exception
     */
    virtual ~memoryException() {}

    /**
     * @brief Report what has gone wrong
     * @return The report
     */
    inline virtual const char *what() const throw() {
        return ( string ( "Memory exception: " ) + details ).c_str();
    }

private:
    string details;

};


/**
 * @brief An exception for when something totally unexpected happened
 */
class unexpectedStateException : rambrainException
{

public:
    /**
     * @brief Create a new exception
     * @param details Optionally provided details about the current state of affairs
     */
    unexpectedStateException ( const string &details ) : details ( details ) {}
    /**
     * @brief Destroy the exception
     */
    virtual ~unexpectedStateException() {}

    /**
     * @brief Report what has gone wrong
     * @return The report
     */
    inline virtual const char *what() const throw() {
        return ( string ( "Unexpected state detected: " ) + details ).c_str();
    }

private:
    string details;

};


/**
 * @brief An exception class for when code is used which is not fully finished developping
 *
 * Should not be encountered anymore as soon as the library is live. Can be encountered if new features are partially developped or so.
 */
class unfinishedCodeException : rambrainException
{

public:
    /**
     * @brief Create a new exception
     * @param details Optionally provided details about the current state of affairs
     */
    unfinishedCodeException ( const string &details ) : details ( details ) {}
    /**
     * @brief Destroy the exception
     */
    virtual ~unfinishedCodeException() {}

    /**
     * @brief Report what has gone wrong
     * @return The report
     */
    inline virtual const char *what() const throw() {
        return ( string ( "Unfinished code section encountered: " ) + details ).c_str();
    }

private:
    string details;

};

}

#endif // EXCEPTIONS_H

