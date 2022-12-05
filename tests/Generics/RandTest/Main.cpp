#include <iostream>
#include <vector>
#include <list>
#include <map>

#include <Generics/Rand.hpp>
#include <Generics/RandomSelect.hpp>

struct Weight
{
  int operator()(int i) const
  {
    return i;
  }
};

int main()
{
  typedef std::list<int> IntList;
  IntList lst;
  lst.push_back(1);
  lst.push_back(2);
  lst.push_back(3);
  std::map<int, int> res;

  for(int i = 0; i < 10000; ++i)
  {
    IntList::const_iterator it =
      Generics::random_select<int>(lst.begin(), lst.end(), Weight());
    
    if(it == lst.end())
    {
      std::cerr
        << "random_select can't return end iterator for non empty seq "
           "(where exists element with weight != 0)."
        << std::endl;
      
      return 1;
    }
    res[*it]++;
  }
  
  if(res[1] == 0 || res[2] == 0 || res[3] == 0)
  {
     std::cerr << "Strange ";
  }

  std::cout
    << "result of random_select: " << std::endl
    << "1 selected: " << res[1] << std::endl
    << "2 selected: " << res[2] << std::endl
    << "3 selected: " << res[3] << std::endl;
    
  for(int i = 0; i < 10000; ++i)
  {
    if(Generics::safe_rand(0, 1) > 1)
    {
      std::cerr << "rand(0, 1) return value >1." << std::endl;
      return 1;
    }
  }
  
  return 0;
}
