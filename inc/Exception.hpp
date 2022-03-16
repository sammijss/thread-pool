#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include<string>

class Exception : public std::exception {
    private:
        std::string m_error;

    public:
        Exception(const std::string& error) : m_error(error) {}
        virtual ~Exception() = default;

        Exception(const Exception&) = delete;               /* Copy constructor */
        Exception& operator=(const Exception&) = delete;    /* Assignment operator */

        Exception(Exception&&) = delete;                    /* Move constructor */
        Exception& operator=(Exception&&) = delete;         /* Move assignment operator */

        const char* what() const throw () override { return m_error.c_str(); }
};
#endif
