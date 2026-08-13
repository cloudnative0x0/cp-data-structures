# LinkedList

<p style="text-align: left">
  <a href="#русский">Русский</a> ・ <a href="#english">English</a>
</p>

---

## Русский

Список — это цепочка узлов. Каждый узел хранит значение и два указателя: на предыдущий узел и на следующий. Никакого массива внутри нет, элементы не лежат подряд в памяти — они разбросаны как попало, а порядок задаётся именно этими указателями.

Из этого вытекают два свойства, о которых стоит помнить, когда выбираешь список вместо массива или вектора:

```cpp
#include "LinkedList.hpp"

cp::LinkedList<int> list = {1, 2, 3};
```

- **Вставить или удалить элемент где угодно — дёшево.** Если у вас уже есть итератор на нужное место, вставка или удаление — это переставить пару указателей, O(1), независимо от того, сколько всего элементов в списке.
- **Обратиться к элементу по номеру — дорого.** Нет никакого `list[50]`. Чтобы дойти до пятидесятого элемента, нужно пройти через предыдущие сорок девять один за другим, O(n).

Если вам нужен быстрый доступ по индексу — берите `std::vector`. Если нужно много вставлять и удалять в середине, особенно большие объекты, где копирование дорого, — список подходит лучше.

### Как это устроено внутри

#### Кольцо с "часовым" узлом

Обычно список хранят как `head` и `tail`, и почти в каждой операции приходится отдельно обрабатывать случай пустого списка, случай вставки в начало и случай вставки в конец. Здесь это устроено иначе: список замкнут в кольцо, и в этом кольце есть один служебный узел — `sentinel_` — который не хранит значения, а просто обозначает границу между концом списка и его началом.

```
sentinel_ → [1] → [2] → [3] → sentinel_ → [1] → ...
```

`begin()` — это первый настоящий элемент, `end()` — это сам `sentinel_`. Когда список пуст, `sentinel_` указывает сам на себя, и `begin() == end()` получается автоматически, без единой проверки "а пуст ли список".

Благодаря этому трюку вставка перед первым элементом, вставка перед последним и вставка в середину — буквально один и тот же код. Никаких особых случаев для головы и хвоста.

#### Память под узлы берётся не через new/delete напрямую

Каждый раз выделять и освобождать память под один узел через `new`/`delete` — довольно медленно. Вместо этого используется пул: память выделяется сразу большими кусками по 1024 узла, и когда узел удаляется, его память не возвращается системе, а просто помечается как свободная и переиспользуется под следующий новый узел.

Практический смысл: если вы часто вставляете и удаляете элементы, список не будет постоянно дёргать аллокатор — переиспользование памяти происходит внутри пула, быстро.

Из этого следует один нюанс: память, которую список один раз занял, не возвращается операционной системе, пока программа не завершится. Для большинства задач это не проблема, но если вы создаёте и уничтожаете очень много списков с очень большими объектами — стоит иметь это в виду.

### Как пользоваться

#### Создание

```cpp
cp::LinkedList<int> a;                     // пустой список
cp::LinkedList<int> b = {1, 2, 3, 4};       // из списка значений
cp::LinkedList<int> c(5, 0);                // пять нулей
cp::LinkedList<int> d(b.begin(), b.end());  // копия диапазона
cp::LinkedList<int> e = b;                  // копия
cp::LinkedList<int> f = std::move(b);       // перемещение, b становится пустым
```

#### Добавление и удаление элементов

```cpp
cp::LinkedList<int> list = {2, 3, 4};

list.push_back(5);      // 2 3 4 5
list.push_front(1);     // 1 2 3 4 5
list.pop_back();        // 1 2 3 4
list.pop_front();       // 2 3 4

auto it = list.begin();
++it;                              // указывает на второй элемент
list.insert(it, 100);              // вставить перед ним
list.erase(it);                    // удалить сам этот элемент
```

`push_back`/`push_front` добавляют в конец/начало. `insert(pos, value)` вставляет перед позицией `pos`. `erase(pos)` удаляет элемент в этой позиции. Всё это работает за O(1), если позиция у вас уже есть в виде итератора.

#### Обход

```cpp
for (int x : list) {
    // обычный проход от начала до конца
}

for (auto it = list.rbegin(); it != list.rend(); ++it) {
    // проход в обратном порядке
}
```

#### Доступ к краям

```cpp
list.front();   // первый элемент
list.back();    // последний элемент
list.empty();   // пуст ли список
list.size();    // сколько элементов
```

Обратите внимание: `front()`/`back()` ничего не проверяют сами — если вызвать их на пустом списке, это ошибка на совести вызывающего кода, точно как и в `std::list`.

#### Полезные алгоритмы уже встроены

```cpp
list.sort();                                   // отсортировать
list.reverse();                                // развернуть список
list.unique();                                  // убрать соседние дубликаты
list.remove(3);                                 // убрать все элементы, равные 3
list.remove_if([](int x) { return x < 0; });    // убрать все отрицательные
```

`sort()` сортирует за O(n log n) и не выделяет память под сами элементы — переставляются узлы, а не копируются значения. Это особенно выгодно, если `T` — что-то тяжёлое для копирования.

#### Перенос кусков между списками — splice

```cpp
cp::LinkedList<int> a = {1, 2, 3};
cp::LinkedList<int> b = {100, 200};

a.splice(a.begin(), b);   // весь b переезжает в начало a, b становится пустым
// a: 100 200 1 2 3
```

`splice` переносит элементы из одного списка в другой без копирования — просто перевешивает указатели. Можно перенести весь список, один элемент или диапазон:

```cpp
a.splice(pos, other);                    // весь список other
a.splice(pos, other, other_it);          // один элемент
a.splice(pos, other, first, last);       // диапазон [first, last)
```

### Таблица операций

| Операция | Что делает | Сложность |
|---|---|---|
| `push_back` / `push_front` | добавить в конец / начало | O(1) |
| `pop_back` / `pop_front` | убрать с конца / начала | O(1) |
| `insert(pos, value)` | вставить перед позицией | O(1) |
| `erase(pos)` | удалить элемент | O(1) |
| `front()` / `back()` | первый / последний элемент | O(1) |
| `size()` / `empty()` | размер / пустота | O(1) |
| доступ по индексу | — | не поддерживается вообще |
| `splice` | перенести кусок из другого списка | O(1) (кроме подсчёта размера при переносе между разными списками) |
| `sort()` | отсортировать | O(n log n) |
| `reverse()` | развернуть | O(n) |
| `remove` / `remove_if` / `unique` | удалить по условию | O(n) |

### Когда это не подходит

- Нужен быстрый доступ по индексу — берите `std::vector`.
- Элементы маленькие (например, `int`), а список короткий — накладные расходы на указатели (два указателя на каждый `int`) и на непоследовательное расположение в памяти (кэш-промахи при обходе) часто делают связный список медленнее вектора даже там, где по теории он должен выигрывать.
- Нужен доступ из нескольких потоков одновременно — список это не поддерживает, синхронизацию нужно делать снаружи.

---

## English

A list is a chain of nodes. Each node stores a value and two pointers: one to the previous node, one to the next. There's no array underneath — elements don't sit next to each other in memory, they're scattered wherever, and order is defined purely by those pointers.

Two properties follow from this, worth keeping in mind when choosing a list over an array or a vector:


```cpp
#include "LinkedList.hpp"

cp::LinkedList<int> list = {1, 2, 3};
```

- **Inserting or removing anywhere is cheap.** If you already have an iterator to the right spot, insertion or removal is just relinking a couple of pointers — O(1), regardless of how many elements the list holds.
- **Reaching an element by its position is expensive.** There's no `list[50]`. To get to the fiftieth element you have to walk through the previous forty-nine, one by one — O(n).

If you need fast index-based access, use `std::vector`. If you're inserting and removing a lot in the middle, especially with objects that are expensive to copy, a list is the better fit.

### How it works internally

#### A ring with a sentinel node

Lists are usually stored as `head` and `tail`, and nearly every operation ends up handling the empty-list case, the insert-at-front case, and the insert-at-back case separately. Here it works differently: the list is closed into a ring, and inside that ring sits one housekeeping node — `sentinel_` — which doesn't hold a value, it just marks the boundary between the end of the list and its start.

```
sentinel_ → [1] → [2] → [3] → sentinel_ → [1] → ...
```

`begin()` is the first real element, `end()` is `sentinel_` itself. When the list is empty, `sentinel_` points at itself, and `begin() == end()` follows automatically, with no "is the list empty" check anywhere.

Because of this, inserting before the first element, before the last one, and in the middle are literally the same code path. No special cases for head or tail.

#### Node memory doesn't come straight from new/delete

Allocating and freeing memory for a single node through `new`/`delete` every time is fairly slow. Instead a pool is used: memory is allocated in big chunks of 1024 nodes at once, and when a node is erased its memory isn't handed back to the system — it's just marked as free and reused for the next new node.

The practical effect: if you insert and remove elements often, the list doesn't keep hitting the allocator — memory gets recycled inside the pool, quickly.

One consequence follows from this: memory the list has once claimed isn't returned to the operating system until the program ends. For most tasks that's not an issue, but if you're creating and destroying a very large number of lists holding very large objects, it's worth being aware of.

### How to use it

#### Creating a list

```cpp
cp::LinkedList<int> a;                     // empty list
cp::LinkedList<int> b = {1, 2, 3, 4};       // from a list of values
cp::LinkedList<int> c(5, 0);                // five zeros
cp::LinkedList<int> d(b.begin(), b.end());  // copy of a range
cp::LinkedList<int> e = b;                  // copy
cp::LinkedList<int> f = std::move(b);       // move, b becomes empty
```

#### Adding and removing elements

```cpp
cp::LinkedList<int> list = {2, 3, 4};

list.push_back(5);      // 2 3 4 5
list.push_front(1);     // 1 2 3 4 5
list.pop_back();        // 1 2 3 4
list.pop_front();       // 2 3 4

auto it = list.begin();
++it;                              // points to the second element
list.insert(it, 100);              // insert before it
list.erase(it);                    // remove that element itself
```

`push_back`/`push_front` add to the end/front. `insert(pos, value)` inserts before `pos`. `erase(pos)` removes the element at that position. All of this runs in O(1) as long as you already hold the position as an iterator.

#### Traversal

```cpp
for (int x : list) {
    // regular front-to-back pass
}

for (auto it = list.rbegin(); it != list.rend(); ++it) {
    // reverse pass
}
```

#### Accessing the ends

```cpp
list.front();   // first element
list.back();    // last element
list.empty();   // is the list empty
list.size();    // how many elements
```

Note: `front()`/`back()` don't check anything themselves — calling them on an empty list is on the caller, exactly as with `std::list`.

#### Algorithms already built in

```cpp
list.sort();                                   // sort
list.reverse();                                // reverse the list
list.unique();                                  // drop adjacent duplicates
list.remove(3);                                 // remove every element equal to 3
list.remove_if([](int x) { return x < 0; });    // remove every negative element
```

`sort()` runs in O(n log n) and doesn't allocate memory for the elements themselves — nodes get rearranged, values don't get copied. That's especially useful when `T` is expensive to copy.

#### Moving chunks between lists — splice

```cpp
cp::LinkedList<int> a = {1, 2, 3};
cp::LinkedList<int> b = {100, 200};

a.splice(a.begin(), b);   // all of b moves to the front of a, b becomes empty
// a: 100 200 1 2 3
```

`splice` moves elements from one list into another without copying — it just rewires pointers. You can move the whole list, a single element, or a range:

```cpp
a.splice(pos, other);                    // the entire other list
a.splice(pos, other, other_it);          // a single element
a.splice(pos, other, first, last);       // a range [first, last)
```

### Operation table

| Operation | What it does | Complexity |
|---|---|---|
| `push_back` / `push_front` | add to the end / front | O(1) |
| `pop_back` / `pop_front` | remove from the end / front | O(1) |
| `insert(pos, value)` | insert before a position | O(1) |
| `erase(pos)` | remove an element | O(1) |
| `front()` / `back()` | first / last element | O(1) |
| `size()` / `empty()` | size / emptiness | O(1) |
| index-based access | — | not supported at all |
| `splice` | move a chunk from another list | O(1) (except for the size count when moving between different lists) |
| `sort()` | sort | O(n log n) |
| `reverse()` | reverse | O(n) |
| `remove` / `remove_if` / `unique` | conditional removal | O(n) |

### When this isn't the right choice

- You need fast index-based access — use `std::vector`.
- The elements are small (like `int`) and the list is short — the overhead of two pointers per element and the scattered memory layout (cache misses while walking the list) often make a linked list slower than a vector even in cases where it should theoretically win.
- You need concurrent access from multiple threads — the list doesn't support this, synchronization has to be handled outside it.