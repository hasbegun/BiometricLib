#include "Masek.h"
#include "portability.h"

int Masek::fix(double x)
{
    int ret;	
    ret = (int)x;	
    return ret;
}


int Masek::roundND(double x)
{
    int ret;
    if (x >= 0.0)
        ret = (int)(x+0.5);
    else
        ret = (int)(x-0.5);
    return ret;
}


void Masek::printfilter(Masek::filter *mfilter, char *filename)
{
    FILE *fid;
    int i, j;
    
    fid = fopen(filename, "w");
    for (i = 0; i<mfilter->hsize[0]; i++)
        for (j = 0; j<mfilter->hsize[1]; j++)
        {
            if (portability::Math::IsNaN(mfilter->data[i*mfilter->hsize[1]+j])) // Lee: Renamed from 'isnan'
                fprintf(fid, "%d %d NaN\n", i+1, j+1);
            else
                fprintf(fid, "%d %d %f\n", i+1, j+1, mfilter->data[i*mfilter->hsize[1]+j]);
        }
    fclose(fid);
    
}

void Masek::printimage(Masek::IMAGE *m, char *filename)
{
    FILE *fid;
    int i, j;
    
    fid = fopen(filename, "w");
    for (i = 0; i<m->hsize[0]; i++)
        for (j = 0; j<m->hsize[1]; j++)
        {
            fprintf(fid, "%d %d %d\n", i+1, j+1, m->data[i*m->hsize[1]+j]);
        }
    fclose(fid);
    
}
