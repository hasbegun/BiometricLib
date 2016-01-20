#ifndef PORTABILITY_H
#define PORTABILITY_H


namespace portability
{

class Math
{
public:
    static bool IsNaN(const double x);
    static double GetNaN();
private:
    Math();
};

}


#endif // PORTABILITY_H
