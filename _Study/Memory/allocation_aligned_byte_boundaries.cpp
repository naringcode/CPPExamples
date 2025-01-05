// Update Date : 2024-12-27
// OS : Windows 10 64bit
// Program : Visual Studio 2022
// Version : C++20
// Configuration : Debug-x64, Release-x64

#include <iostream>
#include <memory>

using namespace std;

int main()
{
    // 동적할당의 바이트 경계를 확인하기 위한 코드
    int cntA[0x10]{ 0 };
    int cntB[0x10]{ 0 };
    
    for (int i = 0; i < 100'000; i++)
    {
        char* ch = new char{ 'C' };
    
        uint64_t ptr = reinterpret_cast<uint64_t>(ch);
        cntA[ptr & 0xf]++; // 8바이트 경계를 맞춘다면 cntA[0x8]을 대상으로도 카운팅이 되어야 함.
    
        ptr = ptr >> 4; // 16진수 자릿수 하나 내리기
        cntB[ptr & 0xf]++;
    }
    
    for (int i = 0; i < 0x10; i++)
    {
        cout << cntA[i] << '\n';
    }
    
    cout << '\n';
    
    for (int i = 0; i < 0x10; i++)
    {
        cout << cntB[i] << '\n';
    }

    // !! Windows 64비트 환경에서 Visual Studio 2022로 위 코드를 실행하면 배열을 8바이트가 아닌 16바이트 경계에 맞춰서 할당함. !!

    // 정리하기
    //
    // 16바이트 경계에 맞춰서 할당된다는 것은?
    // 쉽게 생각해서 주소가 0x0000000000000000부터 시작한다면 이런 방식으로 할당된다는 것을 뜻한다.
    // 0x0000000000000000
    // 0x0000000000000010 (16)
    // 0x0000000000000020 (32)
    // 0x0000000000000030 (48)
    // 0x0000000000000040 (64)
    //
    // 8바이트 경계를 맞춰서 할당하면 다음과 같이 할당될 것이다.
    // 0x0000000000000000
    // 0x0000000000000008 (8)
    // 0x0000000000000010 (16)
    // 0x0000000000000018 (24)
    // 0x0000000000000020 (32)
    //
    // CPU 아키텍처에 따라 포인터의 메모리 정렬을 맞추지 않으면 성능 저하나 오류가 발생할 수 있다.
    // 하지만 발상을 전환하면 경계 주소 사이사이의 중간 영역에 비트 플래그 속성을 부여하거나 직접적으로 활용할 수 있다.
    // 0x0000000000000001 ~ 0x000000000000000F : 16바이트 경계를 맞춰서 할당할 경우 중간 주소
    // 0x0000000000000001 ~ 0x0000000000000007 : 8바이트 경계를 맞춰서 할당할 경우 중간 주소
    //
    // 이론상으로 다음과 같이 비트 플래그를 설정해서 사용하는 것이 가능하다는 뜻이다.
    // ptr = ptr | 0x0000000000000001
    // ptr = ptr | 0x0000000000000002
    // ptr = ptr | 0x0000000000000004
    // ptr = ptr | 0x0000000000000008
    //
    // 하지만 이처럼 메모리에 직접적으로 플래그를 박는 방식은 CPU 아키텍처에 따라서 혹은 운영체제의 메모리 관리 시스템 체계에 따라서 오류를 야기할 수 있다.
    // 이런 방식을 쓰고 싶다면 아래 기술된 방식처럼 주소를 값으로 캐스팅하여 활용한 다음 적당한 시점에 주소로 복원하는 방식을 택하도록 하자.
    //
    // 경계를 침범하지 않는다면 주소를 값으로 캐스팅한 다음 중간 주소(하위 비트)를 원하는 방식으로 사용할 수 있다.
    // !! 다만 이 경우에는 기본 포인터를 원래대로 복원할 수 있어야 함. !!
    //
    // code)---------------------------------------------
    // mask = ~(0x000000000000000F); // 16바이트 경계일 경우 포인터 값을 파악하기 위한 마스크
    //
    // ptrAsVal = reinterpret_cast<uint64_t>(ptr); // 포인터 주소를 값으로 캐스팅
    // ptrAsVal++; // 포인터 값이 위치한 경계을 침범하지 않는 선에서 활용하는 것이 가능
    // ptrAsVal++;
    // ....
    //
    // restoredPtr = reinterpret_cast<Type*>(ptrAsVal & mask); // 포인터 값으로 복원
    // --------------------------------------------------
    // 
    // 메모리 주소 경계 사이의 빈 부분을 대상으로 비트 플래그와 같은 연산을 적용하는 것이 불가능한 것은 아니지만
    // 가능하면 안정성과 유지보수 측면에서 메모리 정렬을 깨는 것은 좋지 않다.
    // 
    // 굳이 사용하겠다고 하면 포인터를 대상으로 연산하지 말고 이를 값으로 보관한 다음 연산을 진행하는 것이 좋다.
    // 값 자체는 포인터가 아니기에 메모리 정렬을 깨지 않으며 이렇게 사용한 이후 나중에 값을 원래의 포인터로 복원할 수 있어야 한다.
    // 
    // 참고로 Windows의 가상 메모리는 상위 2바이트(16비트)를 비워두고 있으니 이걸 활용하는 것도 나쁘지 않은 방법이다.
    // 실제로 MS가 제공하는 SList를 보면 가상 메모리의 비워지는 상위 2바이트 활용해서 Lock-Free 자료구조를 구현하고 있다.
    // !! 메모리 정렬의 경계와 가상 메모리의 빈 영역은 다른 개념이니 혼동하면 안 됨. !!
    // 
    // 가상 메모리 시스템에서 비워지는 상위 비트를 활용하고자 할 경우 경계 대상이 되는 나머지 하위 비트는 가급적 건드리지 않아야 한다(나중에 복원할 때 힘듦).
    //

    return 0;
}
