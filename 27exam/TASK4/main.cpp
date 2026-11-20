#include <cstddef>
#include<iostream>
#include<cstdint>//使用uint64_t来表示无符号整数，相较于std::size_t和unsigned int,其特点是无论在什么操作系统下(32位、64位等)，其所占位数固定为64位,而其他类型则会因系统的不同改变其位数
#include <type_traits>

//w:wide   h:height
struct CameraFrame 
{
    int w, h;
    const unsigned char* buf; //buffer缓冲区，代表从相机传感器直接读出来一整块连续内存，可用y*w + x找到对应像素
};

struct RosImage
{ 
    int w, h, step;
    const unsigned char* data; 
};  // step 为每行字节数，step >= w

struct VideoFrame  
{
    int w, h;
    const unsigned char* yuv; 
};         // 只使用前 w*h 字节（Y 平面）

namespace virt 
{
    //建立一个接口，内含图像的各个参数：宽、高，且能通过（x,y）来取出像素值
    class ImageSource
    {
    public:
        //将析构函数写为虚函数，
        virtual ~ImageSource() = default;
        
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual unsigned char at(int x,int y) const = 0 ;
    };
    
    class CameraSource : public ImageSource
    {
        private:
        CameraFrame f_;
    public:
        explicit CameraSource(CameraFrame f) 
        : f_(f) //初始化列表，将f_初始化为f，即传入数据
        {

        } 

        int width() const override {return f_.w;}
        int height() const override {return f_.h;}
        
        unsigned char at(int x,int y) const override
        {
            return f_.buf[static_cast<std::size_t>(y) * f_.w + x];//用at（）取像素，返回值为（y * width + x）
            //size_t表示无符号整数，static_cast<>表示类型转换
        }
        
    };

    class RosSource : public ImageSource
    {
        private:
        RosImage f_;
    public:
        explicit RosSource(RosImage f) 
        : f_(f) //初始化列表，将f_初始化为f，即传入数据
        {

        } 

        int width() const override {return f_.w;}
        int height() const override {return f_.h;}
        
        unsigned char at(int x,int y) const override
        {
            return f_.data[static_cast<std::size_t>(y) * f_.step + x];//用at（）取像素，返回值为（y * step + x）//step表示每行的像素数，避免一些系统为了像素对齐在每行像素后插入空白字节
        }
    };

    class VideoSource : public ImageSource
    {
        private:
        VideoFrame f_;
    public:
        explicit VideoSource(VideoFrame f) 
        : f_(f) //初始化列表，将f_初始化为f，即传入数据
        {

        } 

        int width() const override {return f_.w;}
        int height() const override {return f_.h;}
        
        unsigned char at(int x,int y) const override
        {
            const int v = static_cast<int>(f_.yuv[static_cast<std::size_t>(y) * f_.w + x]) - 16;//将unsigned char 转化为 int再减去16
            //原因：标准YUV格式中，Y(亮度)的范围为[16,235],16黑，235白，所以为了将其映射到从0开始的范围中，需要将其值减去16
            return static_cast<unsigned char>(std::max(v,0));
        }
    };

    //将每个像素的亮度值加起来，计算其总和
    std::uint64_t preprocess(const ImageSource& src)//src表示待处理原图，由于图像可能很大，使用引用防止图像被复制引起的内存和时间浪费
    {
        std::uint64_t sum = 0;//使用uint64_t类型，防止图像过大在32位系统下sum无法装下亮度值总和

        const int w = src.width();
        const int h = src.height();

        for(int y = 0; y < h;y++)
        {
            for(int x = 0;x<w;x++)
            {
                sum += src.at(x,y);
            }
        }

        return sum;
    }
}

namespace tmpl
{
    namespace detail 
    {
        //using关键字代表别名，这里是别名模板
        template <typename T> using Bare = std::remove_cv_t<std::remove_reference_t<T>>;

        template <typename> inline constexpr bool always_false_v = false;

        template <typename Frame> unsigned char pixel_at(const Frame& f , int x , int y)
        {
            using F = Bare<Frame>;

            if constexpr(std::is_same_v<F,CameraFrame>) //使用constexpr关键词，使不匹配的分支不进行编译，也就不会实例化
            {
                return f.buf[static_cast<std::size_t>(y) * f.w + x];
            }
            else if constexpr (std::is_same_v<F, RosImage>)
            {
                return f.data[static_cast<std::size_t>(y) * f.step + x];
            }
            else if constexpr (std::is_same_v<F, VideoFrame>) 
            {
                const int v = static_cast<int>(f.yuv[static_cast<std::size_t>(y) * f.w +x]) - 16;
                return static_cast<unsigned char>(std::max(v,0));
            }
            else 
            {
                start_assert(always_false_v<F>,"tmpl::preprocess: unsupported frame type");
                return 0;
            }
        }

    }

    template <typename Frame>
    std::uint64_t preprocess(const Frame& f)
    {
        static_assert(std::is_same_v<detail::Bare<Frame>, CameraFrame> || std::is_same_v<detail::Bare<Frame>, RosImage> || std::is_same_v<detail::Bare<Frame>, VideoFrame>,"tmpl::preprocess: unsupported frame type");

        std::uint64_t sum = 0;

        for(int y = 0; y < f.h ;y++)
        {
            for (int x = 0; x < f.w ;x++)
            {
                sum += detail::pixel_at(f, x, y);
            }
        }
        
        return sum;
    }
}







//---------------------以下是完全交由ai写的测试程序----------------------//






//============================================================================
// 自测：用小到能手算的数据，同时检查「两版一致」和「结果正确」
//
// 【K27】匿名 namespace（未命名命名空间）
//   在 namespace { ... } 里定义的名字具有**内部链接**（internal linkage），
//   效果等价于 C 语言的 static：只在当前翻译单元（.cpp）可见。
//   好处：不会和别的 .cpp 里同名的 check() 撞车，也就不会违反 ODR。
//   如果整个项目只有一个 .cpp，用不用都行；但这是好习惯，
//   换成 static 也完全等价。
// ============================================================================

namespace {

// 打印一行的测试结果，返回是否通过。
//
// 【K28】测试函数的设计：两个独立判断，分别报告
//   - same    ：两版结果是否一致（题目硬性要求）；
//   - correct ：是否等于手算的期望值（防止"两版一起错"）。
//   分开判断的好处是失败时能一眼看出是"实现不一致"还是"公式算错了"。
//   参数用 std::uint64_t 而不是 int：和 preprocess 的返回类型保持一致，
//   免得比较时发生隐式转换（还会触发 -Wsign-compare 之类的警告）。
//
//   const char* name 接收字符串字面量：数组形式的字符串会退化成指向首字符的指针。
bool check(const char* name,
           std::uint64_t virt_sum,
           std::uint64_t tmpl_sum,
           std::uint64_t expected) {
    const bool same = (virt_sum == tmpl_sum);
    const bool correct = (virt_sum == expected);
    const bool ok = same && correct;

    // 【K29】std::cout 的链式调用
    //   operator<< 返回的是 std::ostream&（流本身的引用），
    //   所以可以一直 << 接下去，形成一条"管道"。
    //   std::cout 是全局对象，写它不需要 #include 之外的任何初始化。
    //
    //   '\n' 与 std::endl 的区别：
    //     '\n'   只是一个换行字符，不刷新缓冲区；
    //     endl   换行 + flush（强制把缓冲区内容写出去）。
    //   本程序输出量很小，且最后 main 返回时会自动刷新，用 '\n' 就够；
    //   频繁 endl 会在循环里造成很多次无谓的系统调用，是常见的性能坑。
    std::cout << (ok ? "[PASS] " : "[FAIL] ")
              << name
              << "  virt=" << virt_sum
              << "  tmpl=" << tmpl_sum
              << "  expected=" << expected;

    if (!same) {
        std::cout << "  (两版结果不一致)";
    }
    if (!correct) {
        std::cout << "  (与期望值不符)";
    }
    std::cout << '\n';

    return ok;
}

}  // namespace

int main() {
    bool all_ok = true;

    std::cout << "=== TASK4 示例：虚函数版 vs 模板版 ===\n\n";

    // ------------------------------------------------------------------
    // 测试 1：相机 3x2
    //
    //   1 2 3
    //   4 5 6
    //
    // 和 = 1+2+3+4+5+6 = 21
    //
    // 【K31】数组名退化成指针
    //   camera_pixels 的类型是 unsigned char[6]，但当它出现在"需要指针"的
    //   位置时，会自动退化成 &camera_pixels[0]，正好能赋给 const unsigned char*。
    //   注意：数组**不会**记录自己有多长，这个信息只能由 w/h 额外提供 ——
    //   这也是 C 风格接口容易越界的根源。
    //
    //   为什么用 unsigned char 数组而不是字符串：像素是 0~255 的数值，
    //   不是文本。用 char 也能存，但 char 可能是有符号的，比较和打印时
    //   要多留个心眼（见 K33）。
    // ------------------------------------------------------------------
    const int w = 3;
    const int h = 2;

    unsigned char camera_pixels[6] = {1, 2, 3, 4, 5, 6};
    CameraFrame camera_frame{w, h, camera_pixels};
    // 【K4 复习】这是聚合初始化：按 w、h、buf 的声明顺序依次赋值。
    //   写成 CameraFrame camera_frame(w, h, camera_pixels) 在 C++20 之前不行
    //   （聚合类型不能用小括号初始化），所以这里用花括号最稳。

    virt::CameraSource camera(camera_frame);
    // 【K32】生命周期链条，看清楚谁"指着"谁：
    //     camera_pixels（main 的栈数组，6 字节的像素）
    //          ↑ buf 指向
    //     camera_frame（main 的栈对象，只存 w/h/指针）
    //          ↑ 被拷贝进 f_
    //     camera（虚函数版的包装对象，内部只有 f_ 和 vptr）
    //   三者都活到 main 结束，所以没有任何悬垂指针。
    //
    //   反过来，如果 camera_pixels 是某个函数里的局部数组，
    //   函数返回后 camera_frame.buf 就悬空了 —— 这就是 K2 说的生命周期责任。
    //
    //   顺带算一下对象大小（x86-64）：
    //     sizeof(CameraFrame)  = 4 + 4 + 8      = 16 字节
    //     sizeof(CameraSource) = 8(vptr) + 16   = 24 字节
    //   多出来的 8 字节就是虚函数机制的"对象级成本"：一个 vptr。
    //   模板版没有这个 vptr —— 因为它压根没有基类。

    // 顺便演示坐标约定：x 是列，y 是行，(2,1) 是第二行第三列，值为 6。
    //
    // 【K33】为什么必须 static_cast<int>
    //   std::cout 为 char / signed char / unsigned char 都准备了重载，
    //   它们会把值当成**字符**输出。值 6 是一个控制字符（ACK），
    //   直接输出会看到终端上什么都没有（或者打印出乱码）。
    //   转成 int 之后才会走整数重载，打印出 "6"。
    //   新手最常见的疑问之一："为什么我输出像素值，屏幕上是空白？"
    std::cout << "相机 (x=2, y=1) = "
              << static_cast<int>(camera.at(2, 1)) << "（应为 6）\n";

    all_ok = check("Camera 3x2",
                   virt::preprocess(camera),
                   tmpl::preprocess(camera_frame),
                   21) && all_ok;
    // 【K30】为什么写成 all_ok = check(...) && all_ok;
    //   && 从左到右求值，并且**短路**：左边为 false 时右边不再求值。
    //   把 check(...) 放在左边，可以保证它**总会被调用**。
    //   如果写成 all_ok = all_ok && check(...)，一旦 all_ok 变成 false，
    //   后面的 check 就会被短路跳过 —— 输出只剩前几行，后面的测试"人间蒸发"。
    //   测试代码的铁律：**每个用例都要跑、都要打印**，不能一失败就停。
    //
    //   注意 virt::preprocess(camera) 和 tmpl::preprocess(camera_frame)
    //   是两次独立遍历，分别算出同一个和，再交给 check 比较。

    // 再传一次 const 引用，演示模板参数推导：
    // Frame 仍然推导为 CameraFrame（顶层 const 被 const& 吸收）。
    //
    // 【K34】模板参数推导规则（本行是活教材）
    //   函数模板是 template <typename Frame> preprocess(const Frame& f)。
    //   实参类型是 const CameraFrame&，与形参 const Frame& 配对时：
    //     - 引用部分被忽略，只看 const CameraFrame；
    //     - 形参自带的 const 与实参的 const "对上"了，Frame 推导为 CameraFrame，
    //       **不会**变成 const CameraFrame；
    //   所以这里实例化的仍是 preprocess<CameraFrame>，和上面是**同一份代码**，
    //   不会多生成一份。这就是为什么 pixel_at 里还要用 Bare 兜底：
    //   只有**显式**写出 preprocess<const CameraFrame>(...) 才会真的传进 const 类型。
    //
    //   【易错】如果形参写成 preprocess(Frame f)（按值），
    //   传 const CameraFrame& 时 Frame 会被推导成 CameraFrame（顶层 const 被丢弃），
    //   同时还要做一次拷贝。对"描述符"来说拷贝很便宜，但对大对象是灾难——
    //   所以模板参数也建议写成 const&，一条规则管到底。
    const CameraFrame& camera_const_ref = camera_frame;
    all_ok = check("Camera const&",
                   virt::preprocess(camera),
                   tmpl::preprocess(camera_const_ref),
                   21) && all_ok;

    // ------------------------------------------------------------------
    // 测试 2：ROS 3x2，step = 5，行末 padding 填 0xAA（170）
    //
    // 内存布局（每行 3 个有效像素 + 2 个 padding）：
    //   row 0: 1 2 3 AA AA
    //   row 1: 4 5 6 AA AA
    //
    // 正确结果只算 1..6，和 = 21。
    // 如果错写成 y*w+x，会读到 0xAA：
    //   y=0: 下标 0,1,2  -> 1, 2, 3
    //   y=1: 下标 3,4,5  -> 0xAA, 0xAA, 4   ← 全错位了
    //   和 = 1+2+3+170+170+4 = 350，测试立刻发现。
    //
    // 这就是"好测试"的样子：它专门构造了能暴露 bug 的输入，
    // 而不是随便找一组"跑得通"的数据。
    // ------------------------------------------------------------------
    unsigned char ros_pixels[10] = {
        1, 2, 3, 0xAA, 0xAA,
        4, 5, 6, 0xAA, 0xAA
    };
    RosImage ros_frame{w, h, 5, ros_pixels};
    virt::RosSource ros(ros_frame);
    // 0xAA 是十六进制字面量 = 170。这里用十六进制只是为了让"这是填充值"
    // 在源码里一眼可辨（0xAA 是内存调试里常见的"脏数据"填充模式）。

    all_ok = check("ROS 3x2 (step=5)",
                   virt::preprocess(ros),
                   tmpl::preprocess(ros_frame),
                   21) && all_ok;

    // ------------------------------------------------------------------
    // 测试 3：视频 3x2
    //
    // 原始 Y：
    //   0   15  16
    //   17  235 255
    //
    // 调整后（先减 16，低端钳 0）：
    //   0   0   0
    //   1   219 239
    //
    // 和 = 0+0+0+1+219+239 = 459
    //
    // 这组数据是精心挑的，每个值都检查一个边界：
    //   0    -> 低于 16，钳到 0（考察"下限"）
    //   15   -> 刚好差 1，钳到 0（考察"边界前一个"）
    //   16   -> 黑电平，正好变 0（考察"边界本身"）
    //   17   -> 白送 1（考察"边界后一个"）
    //   235  -> 白电平，变 219
    //   255  -> 满值，变 239（考察"**不做**上限钳位"）
    // ------------------------------------------------------------------
    unsigned char yuv_pixels[6] = {0, 15, 16, 17, 235, 255};
    VideoFrame video_frame{w, h, yuv_pixels};
    virt::VideoSource video(video_frame);

    // 演示上限：255 - 16 = 239，不会被压回 219。
    std::cout << "视频 Y=255 -> "
              << static_cast<int>(video.at(2, 1)) << "（应为 239）\n";

    all_ok = check("Video 3x2",
                   virt::preprocess(video),
                   tmpl::preprocess(video_frame),
                   459) && all_ok;

    // ------------------------------------------------------------------
    // 测试 4：1x1 边界
    //
    // 只有一个像素，值为 250，和 = 250。
    // 用来检查循环边界和单像素处理。
    //
    // 【K32 复习】注意这里 buf 指向的是**单个变量**的地址，而不是数组：
    //     CameraFrame one_frame{1, 1, &one_pixel};
    //   在 C++ 里取一个变量的地址，它就成了"只有一个元素的数组"的合法起点，
    //   buf[0] 是合法的。这不算越界 —— 但也仅限 [0]，buf[1] 就是 UB 了。
    //
    // 为什么专门测 1x1：
    //   - 空循环、单元素循环是最容易写错边界（< 写成 <=）的地方；
    //   - 很多算法在"只有一个元素"时会走到特殊分支，这里能确认没走偏。
    // ------------------------------------------------------------------
    unsigned char one_pixel = 250;
    CameraFrame one_frame{1, 1, &one_pixel};
    virt::CameraSource one_source(one_frame);

    all_ok = check("Camera 1x1",
                   virt::preprocess(one_source),
                   tmpl::preprocess(one_frame),
                   250) && all_ok;

    // ------------------------------------------------------------------
    // 汇总
    //
    // 【K35】main 的返回值：0 表示成功，非 0 表示失败
    //   这是 Unix 的约定（进程退出码），CI / shell 脚本靠它判断程序是否正确：
    //       ./task4 && echo "全部通过"
    //   注意**不能**直接写 return all_ok;：
    //     bool 转 int 时 true -> 1、false -> 0，正好和约定反了 ——
    //     全部通过时反而返回 1（失败），一失败却返回 0（成功）。
    //   所以必须写成条件表达式，把 true 映射成 0、false 映射成 1。
    // ------------------------------------------------------------------
    std::cout << '\n' << (all_ok ? "ALL PASS" : "SOME TESTS FAILED") << '\n';
    return all_ok ? 0 : 1;
}





