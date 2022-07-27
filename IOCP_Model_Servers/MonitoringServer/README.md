# Monitoring Server

- Spec

  1. Windows환경에서 개발(IOCP 사용및 Interlocked 사용으로 os 종속적)
  2. c++ 구현
  3. VS 2019

- 목적

  1. 외부망의 모니터링 클라이언트에서 현재 서버 상태를 확인하기 위해 데이터를 모아주는 서버

- 기능

  1. LanServer

     1-1. 서버와 서버 연결

     1-2. Private Network 안에서만 작동하므로 로그인 과정 없음

     1-3. 받은 패킷은 모두 NetServer에 연결된 모니터링 클라이언트로 전달

  2. NetServer

     2-1. 모니터링 클라이언트들과 서버 연결

     2-2. 외부 망에서 접근 가능하므로 NetServer 프로토콜 사용 및 로그인 과정 있음