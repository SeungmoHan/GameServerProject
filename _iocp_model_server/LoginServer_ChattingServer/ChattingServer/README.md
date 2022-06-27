# Chatting Server (Version1)

- OverlappedIO(IOCP) 모델로 만들어진 네트워크 라이브러리를 상속받아 만들어진 채팅서버
- MMO게임 서버의 채팅서버를 가정
- 워커 쓰레드가 직접 채팅 로직까지 관여하는 쓰레드 디자인
- Spec
  1. Windows 환경에서 개발(IOCP 사용 및 Interlocked 사용으로 os 종속적)
  2. c++ 구현
  3. VS 2019
- 기능
  1. 로그인
  2. 섹터 이동
  3. 채팅 요청

- 부재
  1. 로그인 요청을 위한 Redis 미연동으로 인한 세션 인증키 무시



# Chatting Server (Version2)

- OverlappedIO(IOCP) 모델로 만들어진 네트워크 라이브러리를 상속받아 만들어진 채팅서버
- MMO게임 서버의 채팅서버를 가정
- 워커 쓰레드가 직접 채팅 로직까지 관여하는 쓰레드 디자인
- Spec
  1. Windows 환경에서 개발(IOCP 사용 및 Interlocked 사용으로 os 종속적)
  2. c++ 구현
  3. VS 2019

- 기능
  1. 로그인
  2. 섹터 이동
  3. 채팅 요청
  4. Redis DB 연동
