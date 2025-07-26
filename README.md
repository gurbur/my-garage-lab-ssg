<details>
<summary>🇰🇷 한국어</summary>

# My Garage Lab SSG

제 블로그를 위해 C언어로 처음부터 직접 개발한 **Obsidian Vault용 정적 사이트 생성기(Static Site Generator)**입니다. 이 프로젝트는 단순한 결과물 생성을 넘어, 컴파일러, 자료구조, 시스템 프로그래밍 등 컴퓨터 과학의 근본 원리를 실제 프로젝트에 적용하며 깊이 있게 이해하는 것을 목표로 합니다.

## ✨ 주요 기능

  * **Markdown to HTML**: GFM(GitHub Flavored Markdown)과 유사한 문법을 파싱하여 정적 HTML 페이지를 생성합니다. 아직 몇몇 문법(참조 등)은 커버하지 못했지만, 추후 기능 추가 예정입니다.
  * **템플릿 엔진**: `{{ title }}`과 같은 변수와 `{{ component: header }}` 같은 컴포넌트 시스템을 지원하여, 재사용 가능한 레이아웃과 디자인을 쉽게 적용할 수 있습니다.
  * **Obsidian 최적화**:
      * `[[내부 링크]]`나 `![[이미지.png]]` 같은 Obsidian 고유의 링크 문법을 올바른 웹 경로로 자동 변환합니다.
      * Vault의 디렉토리 구조를 기반으로 **동적 사이드바**와 **Breadcrumb**을 자동으로 생성합니다.
  * **고성능 증분 빌드 (Incremental Build)**:
      * 파일 내용의 해시(SHA256)를 캐싱하여, 변경된 파일만 다시 빌드하고 삭제된 파일의 결과물은 자동으로 제거합니다. 이를 통해 대규모 Vault에서도 빠른 빌드 속도를 유지합니다.
  * **유연한 설정**: `config.json` 파일을 통해 사이트 제목, 빌드 경로, 메뉴 등 다양한 옵션을 쉽게 설정할 수 있습니다.

## 🚀 시작하기

### 필요 사항

  * `gcc` 컴파일러
  * `make` 빌드 도구
  * `C` 표준 라이브러리

### 🛠️ 빌드 방법

프로젝트를 컴파일하고 실행 파일을 생성하려면 `make` 명령어를 사용하세요.

1.  **전체 프로젝트 빌드**

    ```bash
    make
    ```

    또는

    ```bash
    make ssg
    ```

    위 명령어를 실행하면 `builds/` 디렉토리에 `ssg` 실행 파일이 생성됩니다.

2.  **테스트 빌드 및 실행**

    ```bash
    make test
    ./run_tests.sh
    ```

3.  **빌드 결과물 삭제**

    ```bash
    make clean
    ```

### 🏃 사용법

빌드된 `ssg` 실행 파일을 사용하여 정적 사이트를 생성할 수 있습니다.

```bash
# 현재 디렉토리의 Obsidian Vault를 기준으로 빌드
./builds/ssg

# 특정 경로의 Obsidian Vault를 기준으로 빌드
./builds/ssg /path/to/your/vault
```

빌드가 성공적으로 완료되면 `config.json`에 지정된 출력 디렉토리(기본값: `ssg_output`)에 결과물이 생성됩니다.

### ⚙️ 설정 (`config.json`)

프로젝트 루트의 `config.json` 파일을 수정하여 사이트의 동작을 제어할 수 있습니다.

**예시 `config.json`:**

```json
{
	"site_title": "My Garage Lab",
	"author": "Your Name",
	"base_url": ".",
	"hard_line_breaks": "true",
	"templates": {
		"default_layout": "post_page_layout",
		"template_dir": "templates"
	},
	"build": {
		"output_dir": "ssg_output",
		"static_dir": "static",
		"image_dir": "images"
	}
}
```

  * `site_title`: 웹사이트의 제목입니다.
  * `hard_line_breaks`: `true`로 설정하면 마크다운에서 엔터 한 번만으로도 줄바꿈(`<br>`)이 적용됩니다.
  * `build.output_dir`: 빌드 결과물이 저장될 디렉토리 이름입니다.
  * `build.static_dir`, `build.image_dir`: 빌드 시 그대로 `output_dir`에 복사될 정적 파일(CSS, JS) 및 이미지 디렉토리의 이름입니다.

</details>

# My Garage Lab SSG

A **Static Site Generator for Obsidian Vaults**, built from scratch in C for my own blog. This project aims to provide a deep understanding of fundamental computer science principles—such as compilers, data structures, and systems programming—by applying them to a real-world project, rather than just producing a final product.

## ✨ Key Features

  * **Markdown to HTML**: Parses GFM-like (GitHub Flavored Markdown) syntax to generate static HTML pages. Not every syntax are covered yet(`>`), but it'll be updated soon.
  * **Template Engine**: Supports variables like `{{ title }}` and a component system like `{{ component: header }}` to easily apply reusable layouts and designs.
  * **Optimized for Obsidian**:
      * Automatically converts Obsidian's unique link syntax, such as `[[Internal Link]]` and `![[Image.png]]`, into valid web paths.
      * Generates a **dynamic sidebar** and **breadcrumbs** based on your Vault's directory structure.
  * **High-Performance Incremental Builds**:
      * Caches the hash (SHA256) of file contents to rebuild only modified files and automatically removes outputs of deleted files. This ensures fast build times even for large Vaults.
  * **Flexible Configuration**: Easily configure various options like site title, build paths, and menus through a `config.json` file.

## 🚀 Getting Started

### Prerequisites

  * `gcc` compiler
  * `make` build tool
  * Standard C Library

### 🛠️ How to Build

Use the `make` command to compile the project and create an executable.

1.  **Build the entire project**

    ```bash
    make
    ```

    or

    ```bash
    make ssg
    ```

    This command will create an `ssg` executable in the `builds/` directory.

2.  **Build and run tests**

    ```bash
    make test
    ./run_tests.sh
    ```

3.  **Clean build artifacts**

    ```bash
    make clean
    ```

### 🏃 Usage

You can use the built `ssg` executable to generate your static site.

```bash
# Build based on the Obsidian Vault in the current directory
./builds/ssg

# Build based on a specific Obsidian Vault path
./builds/ssg /path/to/your/vault
```

Upon successful build, the output will be generated in the directory specified in `config.json` (default: `ssg_output`).

### ⚙️ Configuration (`config.json`)

You can control the site's behavior by modifying the `config.json` file in the project root.

**Example `config.json`:**

```json
{
	"site_title": "My Garage Lab",
	"author": "Your Name",
	"base_url": ".",
	"hard_line_breaks": "true",
	"templates": {
		"default_layout": "post_page_layout",
		"template_dir": "templates"
	},
	"build": {
		"output_dir": "ssg_output",
		"static_dir": "static",
		"image_dir": "images"
	}
}
```

  * `site_title`: The title of your website.
  * `hard_line_breaks`: If set to `true`, single newlines in Markdown will be converted to `<br>` tags.
  * `build.output_dir`: The directory where the build output will be stored.
  * `build.static_dir`, `build.image_dir`: Names of directories for static files (CSS, JS) and images that will be copied as-is to the `output_dir` during the build.

