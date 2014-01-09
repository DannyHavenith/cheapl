/*
 * xpl_application_service.h
 *
 *  Created on: Jan 9, 2014
 *      Author: danny
 */

#ifndef XPL_APPLICATION_SERVICE_H_
#define XPL_APPLICATION_SERVICE_H_
#include <memory>
#include <stdexcept>

namespace xpl
{
class application_service
{
public:
    application_service();
    ~application_service();
private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};
}
#endif /* XPL_APPLICATION_SERVICE_H_ */
