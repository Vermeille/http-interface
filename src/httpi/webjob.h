#pragma once

#include <string>
#include <memory>

#include "job.h"
#include "html/html.h"

class WebJob {
    std::shared_ptr<std::string> res_;

  public:
    WebJob() : res_(std::make_shared<std::string>("empty")) {}
    std::shared_ptr<std::string> page() { return res_; }
    virtual void Do() = 0;
    ~WebJob() = default;
    virtual std::string name() const = 0;

  protected:
    void SetPage(const httpi::html::Html& html) {
        res_ = std::make_shared<std::string>(html.Get());
    }
};

typedef JobPool<WebJob> WebJobsPool;
