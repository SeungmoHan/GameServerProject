# LockFree Algorithm



- Lock의 단점인 경합상황간 블로킹 및 그로 인한 오버헤드 방지 목적
- stack, queue 자료구조 구현
- Spec
  1. Windows환경에서 개발(Interlocked 계열 함수 이용으로 타 os에서는 지원 x)
  2. c++ 구현
  3. VS 2019



# LockFree Memory Pool

- 개발 목적

  1. new delete는 기본적으로 기본 프로세스 힙에서 할당 받기 때문에 여러 쓰레드에서 접근시 Lock이 필수.

  2. Heap의 경우 삭제된 메모리를 바로 디커밋 하는 경우 혹은 삭제된 메모리를 재사용 하기 때문에 이로 인한 문제 발생 방지

  3. Virtual Alloc 호출로 커널 전환 발생 및 이로 인한 성능 하락 방지

  4. 메모리풀은 메모리 사용추세를 파악할 수 있지만 오브젝트 풀은 정확한 사용처와 사용량을 바로 파악할 수 있음.

     4-1. 오브젝트 풀이 누수 확인 및 모니터링에 효율적일 것 이라 판단하여 오브젝트 풀로 설계(이름은 MemoryPool 이라고 붙이긴함.)

- Spec
  1. Windows 환경에서 개발(Interlocked 계열 함수 이용으로 타 os에서는 지원 x)
  2. c++ 구현
  3. VS 2019



# LockFree Object Pool TLS(Chunk)

- 개발 목적
  1. LockFree와 Lock은 공통적으로 경합 발생시 동시에 1개의 쓰레드만 접근할 수 있다는 점이있음.
  2. 이론상 LockFree와 Lock의 코드 관점에서만 본다면 큰 로직 차이가 존재하지 않음.
  3. 경합 과정을 줄이는 것이 효율적이라고 판단.
- 기능
  1. LockFreeMemoryPoolTLS는 내부적으로 템플릿 인자가 Chunk인 LockFreeMemoryPool를 가짐
  2. Alloc요청이 들어오면 Chunk내의 하나의 노드를 반환 Chunk가 모두 소진되면 내부의 Pool에게 Chunk를 Alloc받음.
  3. Chunk의 size수 만큼 경합 가능성이 줄어들기 때문에 성능이 개선.
- Spec
  1. Windows 환경에서 개발(Interlocked 계열 함수 이용으로 타 os에서는 지원 x)
  2. c++ 구현
  3. VS 2019
