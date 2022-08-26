| regular expression | token | attribute-value |
|  --- | --- | --- |
| **ws** | - | - |
| if | **IF** | - |
| then | **THEN** | - |
| else | **ELSE** | - |
| **id** | **ID** | pointer to table entry |
| **num** | **NUM** | pointer to table entry |
| < | **RELOP** | **LT** |
| <= | **RELOP** | **LE** |
| = | **RELOP** | **EQ** |
| <> | **RELOP** | **NE** |
| > | **RELOP** | **GT** |
| >= | **RELOP** | **GE** |

- expr
  - expr op term  
  - if expr then expr
  - if expr then expr else expr
  - term
- term
  - id
  - num
  - ( expr )





- expr
  - term expr'  
  - if expr then expr  
  - if expr then expr else expr  
- expr'
  - op expr expr'  
  - **empty**  
- term
  - id 
  - num 
  - ( expr )
