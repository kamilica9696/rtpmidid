/**
 * Real Time Protocol Music Industry Digital Interface Daemon
 * Copyright (C) 2019 David Moreno Montero <dmoreno@coralbits.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <string>
#include <functional>
#include <map>
#include <fmt/format.h>
#include <arpa/inet.h>
#include "./exceptions.hpp"


namespace rtpmidid {
  class mdns {
  public:
    enum query_type_e{
      PTR=12,
      SRV=33,
      A=1,
      AAAA=28,
    };

    struct service{
      std::string label;
      query_type_e type;
      uint32_t ttl;

      service() {};
      service(std::string label_, query_type_e type_, uint32_t ttl_) :
        label(std::move(label_)), type(type_), ttl(ttl_) {}
      virtual std::unique_ptr<service> clone() const{
        throw exception("Not implemented clone for basic DNS.");
      }
      virtual bool equal(const service *other) {
        return false;
      }
      virtual std::string to_string() const{
        return fmt::format("?? Q {} T {} ttl {}", label, type, ttl);
      }
    };
    struct service_a : public service {
      union{
        uint8_t ip[4];
        uint32_t ip4;
      };

      service_a() {}
      service_a(std::string label_, query_type_e type_, uint32_t ttl_, uint8_t ip_[4]) :
        service(std::move(label_), type_, ttl_) {
          ip[0] = ip_[0];
          ip[1] = ip_[1];
          ip[2] = ip_[2];
          ip[3] = ip_[3];
        }
      virtual std::unique_ptr<service> clone() const{
        auto ret = std::make_unique<service_a>();
        *ret = *this;

        return ret;
      }
      virtual bool equal(const service *other_){
        if (const service_a *other = dynamic_cast<const service_a *>(other_)){
          return ip4 == other->ip4;
        }
        return false;
      }
      virtual std::string to_string() const{
        return fmt::format("A Q {} T {} ttl {} ip {}.{}.{}.{}", label, type, ttl,
          uint8_t(ip[0]), uint8_t(ip[1]), uint8_t(ip[2]), uint8_t(ip[3]));
      }
    };
    struct service_srv : public service {
      std::string hostname;
      uint16_t port;

      service_srv() {}
      service_srv(std::string label_, query_type_e type_, uint32_t ttl_, std::string hostname_, uint16_t port_) :
        service(std::move(label_), type_, ttl_),  hostname(std::move(hostname_)), port(port_) {}
      virtual std::unique_ptr<service> clone() const{
        auto ret = std::make_unique<service_srv>();
        *ret = *this;

        return ret;
      }
      virtual bool equal(const service *other_){
        if (const service_srv *other = dynamic_cast<const service_srv *>(other_)){
          return hostname == other->hostname && port == other->port;
        }
        return false;
      }
      virtual std::string to_string() const{
        return fmt::format("SRV Q {} T {} ttl {} hostname {} port {}", label, type, ttl,
          hostname, port);
      }
    };
    struct service_ptr : public service {
      std::string servicename;

      service_ptr() {}
      service_ptr(std::string label_, query_type_e type_, uint32_t ttl_, std::string servicename_) :
        service(std::move(label_), type_, ttl_), servicename(std::move(servicename_)) {}
      virtual std::unique_ptr<service> clone() const{
        auto ret = std::make_unique<service_ptr>();
        *ret = *this;

        return ret;
      }
      virtual bool equal(const service *other_){
        if (const service_ptr *other = dynamic_cast<const service_ptr *>(other_)){
          return servicename == other->servicename;
        }
        return false;
      }
      virtual std::string to_string() const{
        return fmt::format("PTR Q {} T {} ttl {} servicename {}", label, type, ttl,
          servicename);
      }
    };

  private:
    int socketfd;
    struct sockaddr_in multicast_addr;
    // queries are called once
    std::map<std::pair<query_type_e, std::string>, std::vector<std::function<void(const service *)>>> query_map;
    // discovery are called always there is a discovery
    std::map<std::pair<query_type_e, std::string>, std::vector<std::function<void(const service *)>>> discovery_map;
    // I know about all this entries, just in case somebody asks
    std::map<std::pair<query_type_e, std::string>,  std::vector<std::unique_ptr<service>>> announcements;
    // Cache data.
    std::map<std::pair<query_type_e, std::string>,  std::vector<std::unique_ptr<service>>> cache;
  public:
    mdns();
    ~mdns();

    void query(const std::string &name, query_type_e type);
    // Calls once the service function with the service
    void query(const std::string &name, query_type_e type, std::function<void(const service *)>);
    // Calls everytime we get a service with the given parameters
    void on_discovery(const std::string &service, query_type_e type, std::function<void(const mdns::service *)> f);
    // A service is detected, call query and discovery services
    void detected_service(const service *res);

    void announce(std::unique_ptr<service>, bool broadcast=false);
    bool answer_if_known(mdns::query_type_e type_, const std::string &label);
    void send_response(const service &);
    void mdns_ready();
  };
}

namespace std{
  std::string to_string(const rtpmidid::mdns::service_a &s);
  std::string to_string(const rtpmidid::mdns::service_ptr &s);
  std::string to_string(const rtpmidid::mdns::service_srv &s);
}
