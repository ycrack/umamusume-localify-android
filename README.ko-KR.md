# umamusume-localify-android
[umamusume-localify](https://github.com/GEEKiDoS/umamusume-localify) 안드로이드 포팅 버전

[English](README.md)

## 사용법
1. v24 이상의 [Magisk](https://github.com/topjohnwu/Magisk) 설치 후 Zygisk 를 활성화합니다.
2. SafetyNet을 우회하기 위해 [Shamiko](https://github.com/LSPosed/LSPosed.github.io/releases)를 설치합니다. (DenyList 활성화 시 모듈이 동작하지 않습니다.)
2. Magisk 에서 모듈을 설치합니다.
3. `/sdcard/Android/data/jp.co.cygames.umamusume/` 에 `config.json` 파일, `dicts` 에서 참조하는 번역 파일을 넣습니다.

## 설정 앱
json 파일을 직접 수정하지 않고도 설정을 변경할 수 있는 앱을 만들었습니다.

[GitHub 릴리즈](https://github.com/Kimjio/umamusume-localify-android/releases) 또는 [Play Store](https://play.google.com/store/apps/details?id=com.kimjio.umamusumelocalify.settings)에서 설치 가능합니다.

## 참고사항
작업 경로는 `/sdcard/Android/data/jp.co.cygames.umamusume/` 입니다.

(한국어 버전은 `/sdcard/Android/data/com.kakaogames.umamusume/` 입니다.)

## 설정
- `enableLogger` 덮어쓰지 않은 문자열을 `data.txt` 에 출력합니다. (`true` / `false`, 기본값: `false`)
- `dumpStaticEntries` 하드코딩된 문자열을 `data.txt` 에 출력합니다. (`true` / `false`, 기본값: `false`)
  - `enableLogger` 가 활성화돼야 합니다.
- `dumpDbEntries` DB 데이터를 `original_data/` 에 출력합니다. (`true` / `false`, 기본값: `false`)
- `maxFps` 최대 프레임 설정 (`-1` = 기본값 / `0` = 무제한 / `n > 0` = n까지 제한, 기본값: `-1`)
- `uiAnimationScale` UI 애니메이션 배율 변경 (0 < ~, 기본값: `1`)
  - 주의: `0` 으로 설정 시 소프트락이 발생합니다.
- `uiUseSystemResolution` UI, 라이브 자막 등을 시스템 해상도로 렌더링합니다. (`true` / `false`, 기본값: `false`)
- `replaceToBuiltinFont` (`replaceFont`) 게임 내 폰트를 사용하지 않고 유니티 내장 폰트를 사용합니다. (`true` / `false`, 기본값: `false`)
- `replaceToCustomFont` 게임 내 폰트를 사용하지 않고 사용자 지정 폰트를 사용합니다. (`true` / `false`, 기본값: `false`)
- `fontAssetBundlePath` 폰트 애셋 번들 경로 (예: `custom_font/GyeonggiTitle_Medium/font`)
- `fontAssetName` 폰트 애셋 이름 (예: `GyeonggiTitle_Medium.fontsettings`)
- `graphicsQuality` 사전 정의된 그래픽 설정을 변경합니다. (`-1` ~ `4`, 기본값: `-1`)
  - `-1`: 앱에서 결정
  - `0`: `Toon1280`, MSAA OFF
  - `1`: `Toon1280x2`, MSAA x2
  - `2`: `Toon1280x4`, MSAA x4
  - `3`: `ToonFull`, MSAA x8
- `antiAliasing` MSAA 설정을 변경합니다. (`-1`, `0`, `2`, `4`, `8`, 기본값: `-1`)
  - `-1`: 그래픽 설정을 따름
  - `0`: MSAA OFF
  - `2`: MSAA x2
  - `4`: MSAA x4
  - `8`: MSAA x8
- `forceLandscape` 모든 화면을 강제로 가로로 설정합니다. (`true` / `false`, 기본값: `false`)
- `forceLandscapeUiScale` 강제 가로 모드에 사용할 UI 스케일을 설정합니다. (0 < ~, 기본값: `1.8`)
- `uiLoadingShowOrientationGuide` 화면이 회전되었을 때, 로딩 중 가이드를 표시할지 여부를 설정합니다. (`true` / `false`, 기본값: `true`)
- `replaceAssetsPath` 게임에서 다운로드한 애셋을 대체할 애셋들이 담긴 폴더의 경로를 설정합니다.
  - 대체할 애셋 파일의 이름은 원본 애셋 파일의 해시명과 동일해야 합니다.
  - 예: `2FOXNDZ5H52B3E4JMXVAJ4FRMDE3PX7Q` (홈 Footer 텍스처 및 스크립트가 포함된 애셋 (Android))
- `dicts` 번역 파일 목록 (`/sdcard/Android/data/jp.co.cygames.umamusume/` 이후의 번역 파일 경로 목록)

## 알려진 문제
`maxFps` 를 무제한으로 설정 시 게임의 프레임이 60FPS 에서 반토막난 30FPS 으로 재생됩니다.

## 빌드
1. 소스 코드를 다운 받습니다.
2. Android Studio 사용하여 gradle 작업 `:module:assembleRelease` 을 실행해서 컴파일하면, `out` 폴더에 zip 패키지가 생성됩니다.
