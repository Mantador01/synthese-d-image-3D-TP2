
#include <cstdio>
#include <cctype>
#include <cstring>

#include <array>
#include <vector>
#include <algorithm>


unsigned options_find( const char *name, const std::vector<const char *>& options )
{
    for(unsigned i= 0; i < options.size(); i++)
        if(strcmp(name, options[i]) == 0)
            return i;
    
    return options.size();
}

int option_value_or( const char *name, int default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option != options.size())
    {
        int v= 0;
        if(option +1 < options.size())
            if(sscanf(options[option+1], "%d", &v) == 1)
                return v;
    }
    
    return default_value;
}

bool option_flag_or( const char *name, bool default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option != options.size())
        return true;
    
    return default_value;
}

bool option_value_or( const char *name, bool default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option +1 < options.size())
    {
        int v= 0;
        if(sscanf(options[option+1], "%d", &v) == 1)
            return v;
        
        char tmp[1024];
        if(sscanf(options[option+1], "%s", tmp) == 1)
        {
            if(strcmp(tmp, "true") == 0) 
                return true;
            else if(strcmp(tmp, "false") == 0)
                return false;
        }
    }
    
    return default_value;
}

const char *option_value_or( const char *name, const char *default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option +1 < options.size())
        return options[option +1];
    
    return default_value;
}


struct stats
{
    std::vector<float> bench;
};

std::vector<stats> read_bench( const char *filename )
{
    FILE *in= fopen(filename, "rt");
    if(!in)
        return {};
        
    std::vector<stats> columns(5);
    
    char tmp[1024];
    for(;;)
    {
        // charge une ligne du fichier
        if(fgets(tmp, sizeof(tmp), in) == nullptr)
            break;
        
        // brutal et pas tres souple...
        std::array<float, 5> data;
        if(sscanf(tmp, "%f ; %f ; %f ; %f ; %f ", &data[0], &data[1], &data[2], &data[3], &data[4]) != 5
        && sscanf(tmp, "%f %f %f %f %f ", &data[0], &data[1], &data[2], &data[3], &data[4]) != 5)
            break;
        
        for(unsigned i= 0; i < data.size(); i++)
            columns[i].bench.push_back(data[i]);
            
        //~ for(unsigned i= 0; i < data.size(); i++)
            //~ printf("[%u] %f ", i, data[i]);
        //~ printf("\n");
    }
    
    fclose(in);
    
    return columns;
}

float median( const int index, const std::vector<float>& values, const int size=1 )
{
    if(index - size < 0) return 0;
    if(index + size >= int(values.size())) return 0;
    
    std::vector<float> filter;
    for(int i= index - size; i <= index + size; i++)
        filter.push_back(values[i]);
    
    std::sort(filter.begin(), filter.end());
    return filter[size];
}

int main( int argc, char **argv )
{
    std::vector<const char *> options(argv+1, argv+argc);
    
    const char *filename= option_value_or("-i", nullptr, options);
    const char *output_filename= option_value_or("-o", "filtered.txt", options);
    const int filter_radius= option_value_or("--size", 1, options);
    
    if(!filename)
    {
        printf("usage: %s [--size value] -o output_file -i input_file\n", argv[0]);
        return 1;
    }
    
    std::vector<stats> columns= read_bench(filename);
    if(columns.empty())
    {
        printf("[error] reading '%s'...\n", filename);
        return 1;
    }
    
    std::vector<stats> filtered_columns(columns.size());
    for(unsigned i= 0; i < columns.size(); i++)
    for(unsigned k= filter_radius; k + filter_radius < columns[i].bench.size(); k++)
        filtered_columns[i].bench.push_back( median(k, columns[i].bench, filter_radius) );
    
    FILE *out= fopen(output_filename, "wt");
    if(out)
    {
        for(unsigned i= 0; i < filtered_columns[0].bench.size(); i++)
            fprintf(out, "%f ; %f ; %f ; %f ; %f\n", filtered_columns[0].bench[i], filtered_columns[1].bench[i], filtered_columns[2].bench[i], filtered_columns[3].bench[i], filtered_columns[4].bench[i]);
        
        fclose(out);
    }
    
    return 0;
}
