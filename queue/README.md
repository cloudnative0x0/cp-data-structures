# Queue

<p style="text-align: left">
  <a href="#русский">Русский</a> ・ <a href="#english">English</a>
</p>

---

## Русский

Очередь — линейная структура данных, работающая по принципу **FIFO** (First In, First Out): элемент, добавленный первым, извлекается первым. Это полная противоположность стеку.

Пример: если добавить элементы в порядке `1, 2, 3`, то первым выйдет `1`, затем `2`, затем `3`. Порядок выхода в точности повторяет порядок входа.

### Внутреннее устройство

Реализация использует кольцевой буфер на основе вектора `Q`. Три целочисленные переменные управляют состоянием:

- `head` — индекс первого элемента очереди (откуда читает `dequeue` и `peek`)
- `tail` — индекс свободной ячейки, куда запишется следующий `enqueue`
- `count` — текущее количество элементов

Использование счётчика `count` позволяет однозначно различить пустую и полностью заполненную очередь даже при `head == tail`.

При добавлении элемента значение помещается в `Q[tail]`, после чего `tail` сдвигается циклически вперёд: `(tail + 1) % Q.size()`, а `count` увеличивается. При извлечении читается `Q[head]`, затем `head` сдвигается так же, а `count` уменьшается.

Размер вектора фиксируется при создании очереди и определяет её максимальную ёмкость. Попытка добавить элемент в полную очередь или извлечь из пустой приводит к выбрасыванию стандартных исключений — `std::overflow_error` и `std::underflow_error`.

### Операции

| Операция | Сложность | Описание |
|---|---|---|
| `enqueue(x)` | O(1) | добавить элемент в конец очереди |
| `dequeue()` | O(1) | извлечь элемент из начала очереди |
| `peek()` | O(1) | посмотреть первый элемент, не удаляя |
| `isEmpty()` | O(1) | проверка, пуста ли очередь |
| `isFull()` | O(1) | проверка, заполнена ли очередь |
| `size()` | O(1) | текущее количество элементов |
| `capacity()` | O(1) | максимальная ёмкость очереди |

### Сборка и тестирование

```bash
cd cmake-build-debug
cmake --build . --target stress_queue
./stress_queue
```

Корректность проверяется стресс-тестом — реализация сравнивается с эталонной очередью на `std::deque` на большом числе случайных последовательностей операций. Ожидаемое сообщение при успехе: `All stress tests passed successfully!`

---

## English

A queue is a linear data structure that follows the **FIFO** principle (First In, First Out): the element inserted first is the first one to be removed. It is the exact opposite of a stack.

Example: inserting elements in the order `1, 2, 3` will retrieve `1` first, then `2`, then `3`. The order of removal precisely matches the order of insertion.

### Internal layout

The implementation uses a circular buffer backed by a vector `Q`. Three integer variables control the state:

- `head` — index of the first element (where `dequeue` and `peek` read from)
- `tail` — index of the free cell where the next `enqueue` will write
- `count` — current number of elements

The counter `count` makes it possible to distinguish between an empty queue and a full queue, even when `head == tail`.

When an element is added, the value is stored in `Q[tail]`, then `tail` advances circularly: `(tail + 1) % Q.size()`, and `count` increases. When an element is removed, `Q[head]` is read, then `head` advances in the same way, and `count` decreases.

The vector size is fixed at construction and defines the maximum capacity. Trying to enqueue into a full queue or dequeue from an empty one throws standard exceptions — `std::overflow_error` and `std::underflow_error` respectively.

### Operations

| Operation | Complexity | Description |
|---|---|---|
| `enqueue(x)` | O(1) | add an element to the back of the queue |
| `dequeue()` | O(1) | remove and return the front element |
| `peek()` | O(1) | return the front element without removing it |
| `isEmpty()` | O(1) | check whether the queue is empty |
| `isFull()` | O(1) | check whether the queue is at capacity |
| `size()` | O(1) | current number of elements |
| `capacity()` | O(1) | maximum capacity of the queue |

### Build and test

```bash
cd cmake-build-debug
cmake --build . --target stress_queue
./stress_queue
```

Correctness is verified by a stress test — the implementation is compared against a reference queue based on `std::deque` over a large number of randomized operation sequences. On success, the program prints: `All stress tests passed successfully!`

---

<br>

> Очередь не смотрит на важность, не слышит просьб пропустить вперёд. Она знает только одно: кто пришёл раньше, тот и уйдёт раньше. В этом её слепая справедливость.
>
> *A queue does not look at importance, nor does it hear pleas to skip ahead. It knows only one thing: whoever came first leaves first. In that lies its blind justice.*