#pragma once

/*
 * As far as this file is located with parts of GCC C++ standard library
 * and contains some functions copied form it it is distributed under the
 * same licanse as GCC.
 */

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

// Do not change anything for plain C users

// Undef macro horror

#undef min
#undef max
#undef abs
#undef constrain
#undef radians
#undef degrees
#undef sq
#undef round

namespace std {

  template<typename Tp>
  Tp round(Tp d)
  {
    return Tp(d + 0.5);
  }
    
  // So as to stay as much API compatible as possible min, max and abs
  // are not placed in std namespace
  
  template<typename Tp>
  inline const Tp& min(const Tp& a, const Tp& b) {
      return b < a ? b : a;
  }
  
  template<typename Tp>
  inline const Tp& max(const Tp& a, const Tp& b) {
      return b > a ? b : a;
  }
  
  inline double abs(double x) {
      return __builtin_fabs(x);
  }
  
  inline float abs(float x) {
      return __builtin_fabsf(x);
  }
  
  inline long double abs(long double x) {
      return __builtin_fabsl(x);
  }
  
  inline long abs(long i) {
      return labs(i);
  }
  
  inline long long abs(long long x) {
      return x >= 0 ? x : -x;
  }

}

