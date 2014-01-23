/*
 * cheaplservice.h
 *
 *  Created on: Jan 21, 2014
 *      Author: danny
 */

#ifndef CHEAPLSERVICE_H_
#define CHEAPLSERVICE_H_
#include <memory> // for unique_ptr
#include <string>

namespace xpl
{

class message;

class cheapl_service
{
public:
    cheapl_service( const std::string &directoryname, const std::string &soundcardname, const std::string &application_id, const std::string &application_version = "1.0");
    ~cheapl_service();
    void run();
    static void list_cards( std::ostream& output);

private:
    void handle_command( const message &m);
    void scan_files( const std::string &directoryname);

    struct impl;
    std::unique_ptr<impl> pimpl;

    impl& get_impl();
    const impl& get_impl() const;
};

} /* namespace xpl */
#endif /* CHEAPLSERVICE_H_ */
