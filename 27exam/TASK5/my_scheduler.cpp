#include "my_scheduler.hpp"
#include "simulator.hpp"


// 电梯 e 空闲（上一个动作已完成、门已关闭）时调用，返回它的下一个动作。可以在类中保存任意状态
Action MyScheduler::decide(const View& v,int e)
{
    const Car& car = v.cars[e];
    const int floor = car.floor;

    const Dir d = chooseDir(v,e);

    //本层有没有人要下电梯
    if (car.car_call[floor] != -1)//电梯内有人按了当前层
    {
        if (d == Dir::Up)
            return Action::OpenUp;
        if (d == Dir::Down)
            return Action::OpenDown;

        return openHere(v,e);
    }

    //看本层有没有人要上电梯
    if(car.load < car.capacity)
    {
        const bool up = v.hall_up[floor] != -1;//当前楼层的up被按下了
        const bool down = v.hall_down[floor] != -1;//当前楼层的down被按下了

        //当电梯移动方向和按钮方向相同时，开对应的上/下门
        if(d == Dir::Up && up)
            return Action::OpenUp;
        if(d == Dir::Down && down)
            return Action::OpenDown;
        if(d == Dir::None && (up || down))
            return openHere(v,e);
    }

    dir_[e] = d;//储存移动方向

    //下一步的方向是向上,且当前楼层号小于总楼层数时,上行一层
    if(d == Dir::Up && floor < kFloors)
        return Action::Up;//上行一层
    //下一步的方向是向下,且当前楼层号大于1时,下行一层
    if(d == Dir::Down && floor > 1)//下行时需保证楼层数大于1
        return Action::Down;//下行一层

    //当没有需求时，先等一秒
    return Action::Idle;
}   

//实例化openHere,用于判断本层开门时的下一步方向
Action MyScheduler::openHere(const View &v, int e) const
{
    const int floor = v.cars[e].floor;
    const bool up = v.hall_up[floor] != -1;
    const bool down = v.hall_down[floor] != -1;

    //当上下方向都有人呼叫电梯时
    if (up && down)
    {
        //开门后优先向当前电梯的运行方向运行
        if(dir_[e] == Dir::Down)
            return Action::OpenDown;
        if(dir_[e] == Dir::Up)
            return Action::OpenUp;

        //若当前电梯没有运行方向，开门后向离得最近的呼唤楼层方向运行
        if(v.hall_up[floor] <= v.hall_down[floor])
            return Action::OpenUp;
        else return Action::OpenDown;
    }

    //当只有上方有人呼叫时
    if(up)
    {
        return Action::OpenUp;
    }

    //当只有下方有人呼叫时
    if(down)
    {
        return Action::OpenDown;
    }

    //若无人呼叫，下客后保持当前运行方向不变
    if(dir_[e] == Dir::Down)
    {
        return Action::OpenDown;
    }
    else return Action::OpenUp;
}

//实例化chooseDir(v,e)，用于决定1下一步方向
MyScheduler::Dir MyScheduler::chooseDir(const View &v, int e) const
{
    const Car& car = v.cars[e];
    const int floor = car.floor;
    const bool canPickUp = car.load < car.capacity;

    //初始化目标楼层对于电梯所在位置的方向
    bool carAbove = false;
    bool carBelow = false;
    
    //遍历所有楼层，看电梯内哪层被按下，并查看其在上方还是下方
    for(int f = 1;f<= kFloors;f++)
    {
        if(car.car_call[f] == -1)
            continue;
        if(f > floor)
            carAbove = true;
        if(f < floor)
            carBelow = true;
    }

    if(carAbove && !carBelow)
        return Dir::Up;
    if(carBelow && !carAbove)
        return Dir::Down;
    //若上下都有人要去，则维持当前方向
    if(carAbove && carBelow)
    {
        if(dir_[e] != Dir::None)
            return dir_[e];
        else return nearestCarDir(v, e);//若当前电梯没有移动方向，则后续方向为距离最近的目的地的方向
    }

    //满载时不再回应电梯外部召唤，优先看当前电梯内的目的地方向
    if(!canPickUp)
        return Dir::None;////////////////////////////////////////////////////////////////////

    //电梯内没有人按楼层时，就看各楼层的召唤
    bool hallAbove = false;
    bool hallBelow = false;
    for(int f = 1; f < kFloors + 1;f++)
    {
        if(f == floor)//当召唤楼层就是当前层时
            continue;
        if(v.hall_up[f] == -1 && v.hall_down[f] == -1)//当上方和下方都无召唤时
            continue;
        if(f > floor)
            hallAbove = true;
        if(f < floor)
            hallBelow = true;
    }

    //当当前方向为上时，下一步优先看是否有上方的呼唤
    if(dir_[e] == Dir::Up)
    {
        if(hallAbove)
            return Dir::Up;
        if(hallBelow)
            return Dir::Down;
    }
    //当当前方向为下时，下一步优先看是否有下方的呼唤
    else if (dir_[e] == Dir::Down) 
    {
        if(hallBelow)
            return Dir::Down;
        if(hallAbove)
            return Dir::Up;
    }
    //若当前没有方向，则哪边近走哪边
    else
    {
        const int upDistance = nearestHallDistance(v, floor, 1);
        const int downDistance = nearestHallDistance(v, floor, -1);
        if(upDistance < downDistance)
            return Dir::Up;
        if(downDistance < upDistance)
            return Dir::Down;
        if(upDistance == downDistance)
            return Dir::Up;
    }

    return Dir::None;
}

MyScheduler::Dir MyScheduler::nearestCarDir(const View &v, int e) const
{
    const Car& car = v.cars[e];
    const int floor = car.floor;

    for(int step = 1; step <= kFloors ;step++)
    {
        const int up = floor + step;
        const int down = floor - step;

        if(up <= kFloors && car.car_call[up] != -1)
            return Dir::Up;
        if(down >= 1 && car.car_call[down] != -1)
            return Dir::Down;
    }

    return Dir::None;
}

//从当前楼层出发，沿着dir方向找最近的召唤按钮，返回值为距离
int MyScheduler::nearestHallDistance(const View &v, int floor, int dir) const
{
    for(int step = 1;step <= kFloors;step++)
    {
        const int f = floor + dir * step;
        if(f < 1 || f > kFloors)
            break;
        if(v.hall_up[f] != -1 || v.hall_down[f != -1])
            return step;
    }
    return 11111111;//找不到时返回距离无限大
}