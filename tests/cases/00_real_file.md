
야호. 접니다

불과 며칠 만에 다시 이전 작업을 따라잡았따.

# 새로운 Parser

드디어 다시 파서!


Parser는 tokenizer에서 토큰의 리스트 형태로 만든 파일을, **AstNode**라는 자료구조의 형태로 만들어주고, 이들을 문법적인 의미에 맞게 연결하여, 문서 전체를 하나의 AST(Absract Syntax Tree)로 만드는 역할을 합니다.

지난번 글에서 예시로 들었던 Token타입의 list를 가져오면,

```
(Hash)
(Text, " Test Document")
(Newline)
(Text, "This is the first paragraph")
(Dot)
(Newline)
(Backtick)
(Backtick)
(Backtick)
(Text, "c")
(Newline)
(Text, "printf(\"Hello, Code Block!\n\");")
(Backtick)
(Backtick)
(Backtick)
(Newline)
(Dash)
(Text, " list item 1")
(Newline)
(Tab)
(Dash)
(Text, " nested list item")
(Newline)
(Asterisk)
(Text, "italic")
(Asterisk)
(Text, " and ")
(Asterisk)
(Asterisk)
(Text, "bold")
(Asterisk)
(Asterisk)
(Newline)
(EOF)
```

이런 구조였죠?

이제 파서는 이걸 가져다가, AST로 만들어주면 되는겁니다.

아래와 같은 형태가 되겠군요:

```
DOCUMENT
- HEADING1 | data1: "Test Document"
- PARAGRAPH
	- TEXT | data1: "This is the first paragraph."
- CODE_BLOCK | data1: "printf(\"Hellog, Code Block!\\n\");\n" | data2: "c"
- UNORDERED_LIST
	- LIST_ITEM
		- TEXT | data1: "list item 1"
		- UNORDERED_LIST
			- LIST_ITEM
				- TEXT | data1: "nested list item"
- PARAGRAPH
	- ITALIC | data1: "italic"
	- TEXT | data1: " and "
	- BOLD | data1: "bold"
```

이런 식으로, 단순한 토큰들을 엮어, 각 토큰이 문서 내에서 실질적으로 갖는 역할과 의미를 트리 구조로 만들어주면 됩니다.

그럼 시작해보죠!

## 타입

타입 설계는 지난 번에 작성했던 글과 비슷하게, `list_head`를 이용해서, 일종의 parent-sibling 구조로 트리를 만들어줬어요.


지난 번과 바뀐 부분은, 일단 당연하게도 이젠 token의 리스트를, 디스크 I/O가 아닌 메모리 내의 변수를 통해서 받아오도록 되었으니, **문자열 파싱 부분을 아예 구현할 필요가 없어졌다는 점** 이에요. 야호,


따라서 tokenizer의 `Token` 타입을 그대로 가져와서 사용하면 됩니다!


그래서 이번 `AstNode`자료형과 Enumeration은 아래와 같이 생겼습니다.

```c
typedef enum {
	NODE_DOCUMENT = 0,
	NODE_HEADING1,
	NODE_HEADING2,
	NODE_HEADING3,
	NODE_PARAGRAPH,
	NODE_CODE_BLOCK,
	NODE_LINE,
	NODE_ORDERED_LIST,
	NODE_UNORDERED_LIST,
	NODE_LIST_ITEM,
	NODE_IMAGE_LINK,
	NODE_TEXT,
	NODE_ITALIC,
	NODE_BOLD,
	NODE_ITALIC_AND_BOLD,
	NODE_CODE,
	NODE_LINK,
} AstNodeType;

typedef struct {
	struct list_head list;
	struct list_head children;
	
	AstNodeType type;
	char* data1; // main data: string data, code block, etc.
	char* data2; // sub data: code lang, link, etc.
} AstNode;
```

...네, 보면 아시겠지만, enum의 타입명만 조금 수정되었을 뿐, 이전 설계와 완전히 동일합니다!


## 컴파일러의 기본 개념

이번 파서는 코드 구조가 좀 복잡했어요.

기존엔 tokenizer에서 block processing 후 inline procesing하는 과정을 거쳤었죠?

물론 지금은 토크나이저가 단순 토큰화만 진행하기 때문에, 이런 작업을 parser가 처리해주어야 했어요.

그러다 보니, 보통 컴파일러가 컴파일을 수행하기 위해 사용하는 여러 방법론들을 실제로 한 번 따라해봤습니다.

뭐가 있었는지 하나 씩 살펴봐요.

### 1. 구문 분석(Syntax Analusis) & 재귀적 하향 파싱(Recursive Descent)

재귀적 하향 파싱... 줄여서 재하파(?)

이전 단계에서 만들어진 토큰들이 문법에 맞게 잘 배열되었는지를 확인하고, 이 구조를 tree형태로 구성하는 것을 의미합니다.

우리 parser의 전체적인 동작 방식이 이에 해당해요.

Tokenizer에서 만든 token들의 리스트를 기반으로, `AstNode` 구조체를 기반으로 하는 트리 구조를 구성해요.

특히 우리 parser의 경우, 문서 전체 -> 블럭 -> 인라인 순으로, **큰 단위부터 점점 작은 단위 순으로** 처리를 하기 때문에, 특별히 **하향(descent)** 이라는 말이 붙었고,

여기에 또 특별히, **하위 함수에서 재귀형식으로 상위 함수를 반복적으로 호출하는 구조를** 갖도록 설계가 되어있기 때문에, **재귀(recursive)** 라는 이름이 또 붙어요.

### 2. 예측 파싱(Predictive Parsing) & Lookahead

Lookahead... 엿보기?? 이게 정녕 맞는 번역인가(동공지진

예측 파싱 방식은, **다음에 어떤 문법 요소가 올지 미리 엿보고(lookahead), 그에 맞는 규칙을  적용하는** 파싱 방식입니다.

특별히, "앞의 몇 개의 토큰을 엿보는가?"를 기준으로, 한 개를 엿보면 'LL(1) Parser' 등으로 불러요.

우리 parser에서는, `ParserState`라는 상태 구조체를 기반으로, 현재 보고 있는 `current_head`를 참조하도록 하여 `peek_token()`함수로 lookahead를 수행합니다.

쉽게 말해서, 다음 토큰이 만약 `#`라면, `HEADING`타입일 가능성이 높으니, 이를 기반으로 `parse_heading()`이라는 heading 처리 함수로 보내보는 거에요.


뭔가 당연해보이는데 굳이 왜 이름이 붙었나??

제가 수업을 들을 때에는 다른 더 특이한 컴파일러 구조와 구분하여 부르기 위해, 이런 이름을 붙힌 걸로 이해했습니다.(...)

아무튼 이런 식으로, 타입을 예측하여 일단 수행을 해보고, 아니라면 아래의 개념을 수행해요.

### 3. 백트래킹(BackTracking)

백트래킹은 컴파일러의 파서가 토큰을 파싱하는 과정에서, 특정 구문의 시작부를 확인하여 일단 그 구문으로 처리를 시도했다가, 좀 더 뒤의 내용을 확인해보니 그게 아니어서 원래 상태로 돌아오는 것을 말합니다.

그냥 쉽게 간추리면, **"이건가? ...엥, 아니네. 원래대로 돌아가서 다른거로 해봐야지"** ...라고 생각하면 돼요.


별거 아닌 거 같지만, 이게 생각보다 중요합니다.

구현을 많이 단순하게 만들어주거든요.

초기 토큰만 맞으면 일단 시도해보고, 만약 아니면 백트래킹을 일으켜, 처음으로 돌려서 다른방식으로 시도하도록 해서, 별다른 복잡한 생각 없이 만들어도 잘 버그를 일으키지 않는, 견고한 구조를 갖도록 도와줍니다.

우리 parser에서는 이 기법을 굉장히 많이 이용했는데요,

예시로 markdown문법의 `-`의 경우, `---`의 LINE으로 해석될 여지도 있고, `- list`와 같은 UNORDERED LIST로 해석될 여지도 있어요.

그래서 일단은 LINE으로 보내서, 만약 다음 토큰도 Dash가 아니라면 백트래킹을 일으키는 이런 방식을 많이 적용했습니다.


## 코드 구조?

토큰 리스트가 들어오면, 파서는 아래의 큰 흐름을 따릅니다:

1. `peek_token`함수를 통해 현재 토큰의 타입을 확인.
2. `switch`문으로, 토큰 타입에 따라 어떤 유형의 블록인지 분류(HEADING, LINE, LIST, CODE_BLOCK, PARAGRAPH)
3. 개별 파싱 함수들이, 해당 토큰과 그 뒤의 몇 개의 토큰을 뽑아와, 해당 토큰들을 전담하여 처리.

대충 이런 느낌이었습니다.

이렇게 큰 흐름만 말하면 딱 세 개로 요약되지만, 사실 이를 모두 함수로 구현하면 그 수가 좀 많았고, 거기에 문자열 파싱 함수 개개인의 길이와 복잡도도 꽤나 컸어서, 첫 작성 시에 코드는 약 600줄 정도 되었었어요.


여기에 node를 생성하고 지우는 함수나, 토큰 스트림을 탐색하고, 사용하고, 되돌리는 함수들, 그리고 문자열 파싱 과정에서 들여쓰기를 처리하고, 동적 버퍼를 관리하는 함수 등, 꽤나 사이즈가 컸었습니다.

## 리펙토링

그리고 문제는, 이 코드들을 모두 한 파일(...)에 작성했었다는 점이었죠.

이런 크고 아름다운(...) monolitic 구조는, 컴파일이나 실행 시에 효율은 조금 올라갈지 몰라도, 읽기도 어렵고, 수정하기엔 더더욱 어렵더군요.

그래서 리펙토링을 통해, 전체 코드들을 4개의 sub-module형태로 쪼갰습니다.

메모리 관리와 토큰, 노드 등을 관리하는 `parser_utils.c/h`, 실제 진입점이 되는 함수가 있는 `parser.c`, 블럭을 파싱하는 `block_parser.c/h`, 인라인을 파싱하는 `inline_parser.c/h`로 구성했어요.

이를 통해, 유지보수하기도 쉬운 parser 개발, 끝!! 야호

이제 다음은 html_generator...인데, 사실 이미 코드작업을 마쳐버려서(글 작성 날짜가 꽤나 밀림. 어느새 7월 6일...ㅎㅎ), 해당 내용도 그음방 정리해서 다시 들고올게요.

안녕ㅇㅇㄴㅇㅇㄴㅇㄴㄴㅇ

