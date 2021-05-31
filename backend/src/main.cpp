#include "../third_party/httplib.h"
#include "database.hpp"

int main(int argc, char **argv) {
  // db.BatchAddEntry("./arts");
  httplib::Server svr;
  svr.Get("/search", [&](httplib::Request const &req, httplib::Response &res) {
    auto sts = req.get_param_value("sentence");
    auto j = db.Search(sts);
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Cache-Control", "no-cache");
    res.set_content(j.dump(), "application/json");
  });
  svr.listen("0.0.0.0", 8848);
  return 0;
}
