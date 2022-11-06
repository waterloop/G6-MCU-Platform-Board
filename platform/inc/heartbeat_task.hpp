#pragma once
#include "can.hpp"
#include "thread.hpp"


class HeartBeatTask : public Thread {
    void Task() override;
};
