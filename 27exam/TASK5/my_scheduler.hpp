#pragma once
#include "simulator.hpp"
#include <array>

class MyScheduler
{
public:
    Action decide(const View& v,int e);// decide(v, e) 在电梯 e 空闲（上一个动作已完成、门已关闭）时被调用，返回它的下一个动作
private:
    enum class Dir {Up , Down, None};//枚举：电梯的三个方向
    
    std::array<Dir , kElevators> dir_{Dir::None,Dir::None};//储存两电梯的移动方向

    Action openHere(const View& v, int e) const;//当前层选择OpenUp还是OpenDown

    Dir chooseDir(const View& v,int e) const;//选择下一步方向

    Dir nearestCarDir(const View& v,int e) const;//上下都有电梯召唤时，找最近的方向

    int nearestHallDistance(const View& v, int floor , int dir)const;//从floor出发，沿dir方向找最近的召唤按钮，返回距离
};