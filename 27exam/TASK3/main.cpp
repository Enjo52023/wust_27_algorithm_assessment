#include<iostream>
#include<queue>
#include<vector>
#include<string>


static int R,C;//行数，列数

static std::vector<std::string> map;//地图

static int regions = 0;

//起点和终点
int sr = -1,sc = -1;
int gr = -1,gc = -1;

//四个方向
const int dr[4] = {1,0,-1,0};
const int dc[4] = {0,1,0,-1};

//(dr[1],dc[1])

//储存地图上格子的已访问情况
std::vector<std::vector<char>>visited;

//记录格子的可抵达状态
std::vector<std::vector<char>> reachable_cells;

//储存每个格子的代价
std::vector<std::vector<char>> costs_of_cells;

//判断(r,c)是否在地图内
bool inMap(int r,int c)
{
    if(0 <= r && r<R && 0 <= c && c < C)
    {
        return 1;
    }
    else return 0;
}

bool passable(int r,int c)
{
    if(inMap(r,c) && map[r][c] != '#')
    {
        return 1;
    }
    return 0;
}

void dfs(int r,int c)
{
    if(passable(r,c) == 1)
    {
        reachable_cells[r][c]=1;
        visited[r][c] = 1;
    }

    for(int d = 0; d <4 ; d++)
    {
        int nr = r + dr[d];
        int nc = c + dc[d];

        if(!inMap(nr,nc))
        {
            continue;
        }

        if(passable(nr,nc) == 0)
        {
            continue;
        }

        if(visited[nr][nc] == 1)
        {
            continue;
        }

        visited[nr][nc] = 1;
        reachable_cells[r][c]=1;

        dfs(nr,nc);
    }
}



void find_regions(int R,int C)
{
    for(int r = 0 ; r<R ; r++)
    {
        for(int c = 0; c<C ; c++)
        {
            if(visited[r][c]== 0 & map[r][c] != '#')
            {
                regions++;
                dfs(r,c);
                find_regions(R,C);
            }   
        }
    }
}

int heuristic(int r,int c)
{
    return std::abs(gr-r)+std::abs(gc-c);
}

//每个格子的cost计算
void cells_cost(int r,int c)
{
    if(map[r][c] == 'S' || map[r][c] == 'G' || map[r][c] == '.')
    {
        costs_of_cells[r][c] = 1;
    }
    else costs_of_cells[r][c] = map[r][c] - '0';
}

struct State
{
    int f,g,r,c;
    bool operator>(const State& other) const
    {
        return f> other.f;
    }
};

int Astar()
{
    const int INF = 1000000000;

    //记录起点到(r,c)的最小代价
    std::vector<std::vector<int>> dist(R,std::vector<int>(C,INF));

    //建立一个小顶堆，使每一次拓展的节点都是目前看来最有希望的节点
    std::priority_queue<State,std::vector<State>,std::greater<State>> pq;

    dist[sr][sc] = 0;
    pq.push({heuristic(sr,sc),0,sr,sc});

    while(!pq.empty())
    {
        State cur = pq.top();
        pq.pop();

        //如果当前代价不是目前有记录的最短代价，则说明该记录已过期，直接跳过
        if(cur.g != dist[cur.r][cur.c])
        {
            continue;
        }
        
        //找到终点时的当前代价就是最终的最小代价
        if(cur.r == gr && cur.c ==gc)
        {
            return cur.g;
        }

        for(int d = 0; d<4; d++)
        {
            int nr = cur.r + dr[d];
            int nc = cur.c + dc[d];

            if (!passable(nr, nc))
            {
                continue;
            }

            int ng = cur.g + costs_of_cells[nr][nc];

            if(ng < dist[nr][nc])
            {
                dist[nr][nc] = ng;
                pq.push({ng+heuristic(nr, nc),ng,nr,nc});
            }
        }
    }

    //若没有可抵达终点，则返回-1
    return -1;
}

int main()
{
    //输入R,C值及地图
    std::cin >> R >> C;
    map.resize(R);

    //给各个动态数组分配空间
    visited.assign(R, std::vector<char>(C, 0));
    reachable_cells.assign(R, std::vector<char>(C, 0));
    costs_of_cells.assign(R,std::vector<char>(C,0));

    for (int i = 0; i < R; ++i)
    {
       std::cin >> map[i];
    }

    //找到起点S
    
    for(int r =0;r < R; r++)
    {
        for(int c = 0;c<C;c++)
        {
            if (map[r][c] == 'S')
            {
                sr = r;
                sc = c;
            }
        }
    }
    visited[sr][sc] = 1;
    regions++;

    dfs(sr,sc);

    int num_of_reachable_cells = 0;
    for(int r = 0; r <R;r++)
    {
        for(int c = 0; c<C ; c++)
        {
            if(reachable_cells[r][c] == 1)
            {
                num_of_reachable_cells++;
            }
        }
    }

    //-----------------------------求可通行的区域-------------------------------

    find_regions(R,C);

    //--------------------------求最小代价----------------------------------

    //找到终点

    for(int r =0;r < R; r++)
    {
        for(int c = 0;c<C;c++)
        {
            if (map[r][c] == 'G')
            {
                gr = r;
                gc = c;
            }
        }
    }

    //记录每个格子的代价到cost_of_cells
    for(int r = 0;r < R ;r++)
    {
        for(int c = 0; c<C ; c++)
        {
            cells_cost(r,c);
        }
    }

    //确保终点可达再进行A*计算，减少计算浪费
    int min_cost = -1;
    if(reachable_cells[gr][gc] == 1)
    {
        min_cost = Astar();
    }

    std::cout<<"reachable_cells:"<<num_of_reachable_cells<<'\n';
    std::cout<<"regions:"<<regions<<'\n';
    std::cout<<"cost:"<<min_cost<<std::endl;
}
