# Lan Echo Test

- OverlappedIO(IOCP) 모델로 Lan Server Echo Test
- Spec
  1. Windows환경에서 개발(IOCP 사용및 Interlocked 사용으로 os 종속적)
  2. c++ 구현
  3. VS 2019
- 테스트 환경
  1. 더미(5000) recv TPS 50만 유지 상황에서 테스트
- 한계

# Net Echo Test

- OverlappedIO(IOCP) 모델로 만들어진 Thread Block Attach 방식의 Net Server
- Spec
  1. Windows 환경에서 개발
  2. 클라이언트는 윈도우 종속적(Windows API사용 및 Async Select 사용)
  3. c++ 구현
- 기능
  1. Auth Block(데이터 베이스 쿼리 및 인증키가 맞는지 확인하는 로직)
  2. Echo Block
  2. Echo Block를 게임 쓰레드 블럭이라 가정하고 데이터 베이스 쿼리를 게임 로직을 돌리는 쓰레드에서 처리하기에는 부담이 너무 큼.
  2. 추가로 쓰레드간 이동을 확인하기 위한 역할이 다른 쓰레드 블럭 두개를 두었음.
  2. Auth Block을 통과한 유저만 Echo Test로 넘어오고 그렇지 않으면 연결 종료
