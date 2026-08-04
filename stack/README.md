# Stack

<p style="text-align: left">
  <a href="#русский">Русский</a> ・ <a href="#english">English</a>
</p>

---

## Русский

Стек – линейная структура данных, работающая по принципу **LIFO** (Last In, First Out): последний вошедший элемент выходит первым.

Пример: если добавить элементы в порядке `1, 2, 3`, то `3` окажется наверху и будет извлечён первым, следом `2`, затем `1`. Порядок входа и выхода полностью противоположный.

### Внутреннее устройство

Реализация хранит массив `S` и число `top[S]` — индекс последнего добавленного элемента. Активная часть стека — это `S[1..top[S]]`, где:

- `S[1]` — нижний, самый первый добавленный элемент
- `S[top[S]]` — вершина стека, элемент, добавленный последним

Если `top[S] = 0`, стек пуст. Если `top[S] = n` (максимальная вместимость), попытка добавить новый элемент приводит к переполнению — `stack overflow`.

### Операции

| Операция | Сложность | Описание |
|---|---|---|
| `push(x)` | O(1) | добавить элемент на вершину |
| `pop()` | O(1) | снять и вернуть элемент с вершины |
| `peak()` | O(1) | посмотреть вершину без снятия |
| `isEmpty()` | O(1) | проверка на пустоту |
| `isFull()` | O(1) | проверка на переполнение |
| `size()` | O(1) | текущее количество элементов |

### Сборка и тестирование

```bash
cd cmake-build-debug
cmake --build . --target stress_stack
./stress_stack
```

Корректность проверяется stress-тестом — реализация сравнивается со `std::stack` на большом числе случайных последовательностей операций.

---

## English

A stack is a linear data structure that follows the **LIFO** principle (Last In, First Out): the element inserted last is the first one to leave.

Example: inserting elements in the order `1, 2, 3` places `3` on top, so it comes out first, followed by `2`, then `1`. The exit order is the exact reverse of the entry order.

### Internal layout

The implementation keeps an array `S` and a number `top[S]` — the index of the most recently inserted element. The active part of the stack is `S[1..top[S]]`, where:

- `S[1]` is the bottom, the very first element inserted
- `S[top[S]]` is the top of the stack, the most recently inserted element

If `top[S] = 0`, the stack is empty. If `top[S] = n` (the fixed capacity), inserting another element causes an overflow.

### Operations

| Operation | Complexity | Description |
|---|---|---|
| `push(x)` | O(1) | insert an element on top |
| `pop()` | O(1) | remove and return the top element |
| `peak()` | O(1) | look at the top without removing it |
| `isEmpty()` | O(1) | check whether the stack is empty |
| `isFull()` | O(1) | check whether the stack is at capacity |
| `size()` | O(1) | current number of elements |

### Build and test

```bash
cd cmake-build-debug
cmake --build . --target stress_stack
./stress_stack
```

Correctness is checked with a stress test — the implementation is compared against `std::stack` over a large number of randomized operation sequences.

---

<br>

> Стек не помнит, кто был первым. Он помнит только, кто был последним – и отдаёт именно его. Так устроена память толпы, поколений, целых цивилизаций: наверху всегда то, что случилось недавно, а самое первое, изначальное, погребено глубже всего и достаётся последним, если вообще достаётся.
>
> *A stack has no memory of who came first. It only remembers who came last – and gives that one back. This is how the memory of a crowd works, of generations, of entire civilizations: what happened recently always sits on top, while the very first, the original, lies buried deepest and is reached last, if it is reached at all.*