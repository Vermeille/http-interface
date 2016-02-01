#include "job.h"

#include <iostream>
#include <thread>
#include <chrono>

class DummyJob {
    std::shared_ptr<std::string> res_;
    std::chrono::seconds sleep_;

  public:
    DummyJob(int t) : sleep_(t), res_(std::make_shared<std::string>("null")) {}

    void Do() {
        for (int i = 0; i < 10; ++i) {
            res_.reset(new std::string(std::to_string(i)));
            std::this_thread::sleep_for(sleep_);
        }
    }

    auto res() const { return res_; }
};

int main() {
    JobPool<DummyJob> jp;

    auto t1 = jp.StartJob(std::unique_ptr<DummyJob>(new DummyJob(1)));
    auto t2 = jp.StartJob(std::unique_ptr<DummyJob>(new DummyJob(2)));

    for (int i = 0; i < 20; ++i) {
        std::cout << "loop " << i << std::endl;
        jp.foreach_job([](std::pair<const size_t, std::shared_ptr<Job<DummyJob>>>& j) {
                std::cout << j.first << ": " << *j.second->job_data().res() << std::endl;
            });
        std::cout << *jp.GetId(t1)->job_data().res() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return 0;
}
