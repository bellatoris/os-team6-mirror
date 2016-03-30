# OS study

3/30 상문
- **native app**과 **wrt app** 에 대해 공부함
 proj1 에서 launchpad 에 대해 공부할 때 
 Tizen에는 3개의 로더가 각각 다른 어플리케이션을 관장한다고 
 배웠다. 그 중 하나가 Web runtime app이고, 나머지 2개는 native app 
 이었는데 그 차이가 궁금해서 조사해보았다.

 native app 은 play store 나 app store에서 다운받을 수 있는
 기본적인 앱이다. OS 에 맞게 만들기 때문에 핸드폰의 카메라, GPS, 나침반 같은 기능들을 다 사용할 수 있다.

 WRT app 은 website이다. native app처럼 보이긴하지만, HTML5로 쓰여져 있고,
 Browser가 이를 rendering해준다. 그냥 해당 페이지의 북마크를 home screen에 옮겨 놓으면 된다. 이건 online에서만 사용할 수 있다.

 기기 성능을 최대한 끌어내기에는 native app이 좋고, offline환경에서는 native app만 사용가능하다. 속도도 native app이 빠르다. 개발자 입장에서 개발비용 및 유지보수 측면에서는 web app 이 유리하다. 하지만 UI나 UX는 native app이 좋다. 앱 마켓에 native app을 내놓기 위해서는 일련의 절차가 필요하고 수수료 또한  들지만, web app을 아무런 제약이 없다.

 reference : https://www.nngroup.com/articles/mobile-native-apps/ 
