#pragma once

#include <string>
#include <future>
#include <chrono>
#include <mutex>
#include <map>

template <class PackagedJob>
class Job {
    std::unique_ptr<PackagedJob> job_;
    std::future<void> future_;
    std::chrono::system_clock::time_point start_;
    mutable bool finished_ = false;

  public:
    Job(std::unique_ptr<PackagedJob> job)
        : job_(std::move(job)),
        future_(std::async(std::launch::async, &PackagedJob::Do, job_.get())) {
    }

    bool IsFinished() const {
        if (finished_ == false) {
            bool ended = future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            finished_ = ended;
        }
        return finished_;
    }

    PackagedJob& job_data() { return *job_; }
};

template <class PackagedJob>
class JobPool {
    std::map<size_t, std::shared_ptr<Job<PackagedJob>>> jobs_;
    mutable std::mutex jobs_guard_;

  public:
    typedef std::pair<const size_t, std::shared_ptr<Job<PackagedJob>>> job_type;

    size_t StartJob(std::unique_ptr<PackagedJob> pj) {
        std::lock_guard<std::mutex> lk(jobs_guard_);

        size_t id = jobs_.size();
        jobs_.emplace(std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(std::make_shared<Job<PackagedJob>>(std::move(pj))));
        return id;
    }

    template <class F>
    void foreach_job(F&& f) {
        std::lock_guard<std::mutex> lk(jobs_guard_);

        for (auto& x : jobs_) {
            f(x);
        }
    }

    Job<PackagedJob>* GetId(size_t id) {
        std::lock_guard<std::mutex> lk(jobs_guard_);

        auto res = jobs_.find(id);
        if (res == jobs_.end()) {
            return nullptr;
        } else {
            return res->second.get();
        }
    }
};

