# HashMap

<p style="text-align: left">
  <a href="#русский">Русский</a> ・ <a href="#english">English</a>
</p>

---

## Русский

Хеш-таблица — структура данных, которая хранит пары ключ-значение и в среднем даёт O(1) на вставку, поиск и удаление. Цена за это — коллизии: два разных ключа почти неизбежно попадают в один и тот же слот, если ключей много, а слотов мало. Разница между реализациями хеш-таблиц — это, по сути, разница в том, как именно они разруливают эти коллизии.

В проекте пять разных структур, объединённых общим интерфейсом `IBackend`, плюс отдельно стоящая `DirectAddressTable` для случая, когда коллизий вообще не нужно.

### Прямая адресация — `DirectAddressTable.hpp`

Самый простой случай: универсум ключей `U` небольшой, и можно завести массив размера `|U|`, где `T[k]` хранит значение для ключа `k` напрямую, без всякого хеширования. Это классическая прямая адресация (Кормен, Лейзерсон, Ривест, Штайн, глава 11.1).

`slots_` — вектор `std::optional<V>` размера `universe_size`. `Insert`, `Search`, `Delete` работают за честное O(1): просто обращение по индексу, `CheckRange` бросает исключение, если ключ вышел за границы универсума. Плата за скорость — память: таблица всегда занимает `O(|U|)`, даже если реально используется десяток ключей из миллиона. Отсюда и ограничение применимости — это не решение общего назначения, а инструмент для задач, где ключи — это, например, индексы или маленькие целые числа с известной верхней границей.

### Универсальное хеширование — `UniversalHash.hpp`

Проблема любой фиксированной хеш-функции в том, что для неё всегда можно подобрать входные данные, на которых она будет вырождаться в один бакет — противник (или просто неудачный набор данных) знает функцию заранее. Универсальное хеширование решает это, выбирая функцию случайно из семейства при каждом создании или ресайзе таблицы.

Семейство здесь — классическое:

```
h_{a,b}(x) = ((a·x + b) mod p) mod m
```

где `p` — простое число, заведомо больше любого возможного значения `x` (в коде это `2^61 - 1`), `a` выбирается равномерно из `{1, ..., p-1}`, `b` — равномерно из `{0, ..., p-1}`, а `m` — размер таблицы. При каждом вызове `Reseed()` (в конструкторе и в `Rehash`) `a` и `b` перегенерируются заново через `std::mt19937_64`.

Ключевое свойство такого семейства: для любых двух различных `x ≠ y`

```
Pr[h(x) = h(y)] ≤ 1/m
```

вероятность берётся по случайному выбору `a` и `b`, а не по входным данным. Отсюда и вся последующая математика: сколько бы раз ни запускали программу на одних и тех же ключах, для конкретного противника нет способа гарантированно устроить коллизию, потому что функция каждый раз новая.

`KeyHasher<K>` — обёртка поверх `UniversalHash`: сначала произвольный ключ `K` приводится к `uint64_t` через `std::hash<K>`, затем результат прогоняется через универсальную хеш-функцию. Два разных объекта `KeyHasher` (как в открытой адресации, `h1_` и `h2_`) хешируют независимо, с разными случайными `a`, `b`.

### Разрешение коллизий цепочками — `ChainingBackend.hpp`

Каждый бакет или "ведро" — это `std::list<std::pair<K, V>>`. При коллизии элемент просто дописывается в список того же бакета, а не пытается найти себе другое место.

- `Insert`: сначала проверяется коэффициент заполнения `(count_ + 1) / buckets_.size()`, и если он достиг `max_load_factor_` (по умолчанию 1.0), таблица удваивается заранее, до вставки. Затем — линейный проход по бакету в поисках существующего ключа (обновление значения) либо добавление нового узла в конец списка.
- `Search` и `Delete` — линейный проход по конкретному бакету.
- `Grow`: создаётся новый вектор бакетов, хешер пересевается под новый размер (`Rehash`), и все элементы старой таблицы перекладываются в новую — их бакет меняется, потому что хеш-функция теперь другая.

Математика здесь стандартная (Кормен, глава 11.2): при равномерном хешировании и коэффициенте заполнения `α = n/m` ожидаемая длина непустой цепочки — `O(1 + α)`, и это же — ожидаемое время неуспешного поиска. Поскольку в этой реализации таблица удваивается до того, как `α` превысит 1, средняя длина цепочки никогда не растёт бесконтрольно — амортизированная стоимость операции остаётся `O(1)`, а сам ресайз, хоть и стоит `O(n)`, случается всё реже по мере роста таблицы (классический аргумент амортизационного анализа для удвоения).

### Открытая адресация — `OpenAddressingBackend.hpp`

Здесь коллизии разрешаются иначе: все элементы живут в одном массиве `slots_`, и при коллизии ключ ищет себе следующий свободный слот по заранее определённой последовательности проб. Максимальный коэффициент заполнения — `0.7` (`kMaxLoadFactor`), заметно ниже, чем у цепочек: при открытой адресации производительность деградирует резко при приближении `α` к 1, поэтому запас по месту нужен всегда.

Последовательность проб задаётся как `⟨h(k,0), h(k,1), ..., h(k,m-1)⟩`, и в идеале (гипотеза равномерного хеширования) должна быть равновероятной перестановкой всех `m` индексов для каждого ключа. В коде реализованы три стратегии — `Probe(base, step, i)`:

- **Линейное пробирование** — `idx = (base + i) mod m`. Самая дешёвая по вычислениям, но подвержена первичной кластеризации: соседние занятые слоты имеют тенденцию срастаться в длинные последовательности, потому что любой ключ, чей `base` попал внутрь такой последовательности, продолжает её же и удлиняет.
- **Квадратичное пробирование** — `idx = (base + i²) mod m`. Первичной кластеризации нет, но появляется вторичная: два ключа с одинаковым `base` идут по абсолютно одинаковой последовательности проб. Отдельная тонкость: квадратичное пробирование гарантированно обходит все `m` слотов только при определённых `m` (например, `m` — простое число вида `4k+3`, либо специальный подбор констант); при произвольном чётном `m`, как здесь после удвоения, часть слотов теоретически может быть недостижима с конкретного `base` за один проход — на практике `InsertInto` подстраховывается: если полный проход `i = 0..m-1` не нашёл свободного места, таблица просто удваивается ещё раз.
- **Двойное хеширование** — `idx = (base + i·step) mod m`, где `step` считается от независимого второго хешера `h2_`. Это ближе всего к равномерному хешированию из всех трёх схем. Важный нюанс для корректности: чтобы последовательность `i·step mod m` обошла все `m` остатков, шаг обязан быть взаимно прост с `m`. В коде `StepHash` берёт `step = h2(key) mod (m - 1) + 1`, то есть `step` гарантированно лежит в `[1, m-1]`, но взаимная простота с `m` явно не проверяется — при `m`, равном степени двойки (а начальная и все последующие ёмкости в `HashMap` кратны степеням двойки), чётный `step` заставит пробу двигаться только по слотам одной чётности. Это не ломает корректность (алгоритм найдёт своё же значение или свободное место, если оно есть в достижимой части, а если не найдёт — уйдёт в `Grow`), но снижает эффективное покрытие таблицы для части ключей и на практике проявляется как более частые ресайзы, чем в теоретической оценке.

Удаление реализовано через **надгробия** (`Node::deleted = true`), а не физическое освобождение слота. Это принципиально: если при удалении просто занулить слот, следующий поиск по цепочке проб может ошибочно решить, что дальше по последовательности элементов нет, и остановиться раньше, чем найдёт искомый ключ, который на самом деле лежит дальше. Поэтому в `Search` и `Delete` цикл останавливается только на настоящем `nullptr` (слот, в который вообще никогда ничего не клали), а помеченные удалёнными узлы — пропускаются, но не прерывают проход. При вставке, наоборот, первый встреченный слот-надгробие запоминается как кандидат на переиспользование (`first_free`), но проход продолжается до конца, чтобы сначала убедиться, что ключа ещё нет дальше в таблице.

`Grow` пересоздаёт таблицу нужного размера, пересевает оба хешера (`h1_`, `h2_`) и заново вставляет только живые (не помеченные удалёнными) узлы — надгробия, естественно, при ресайзе исчезают, так как каждый элемент вставляется в новую таблицу с нуля.

При `α = 0.7` и равномерном хешировании классические оценки числа проб (Кормен, теорема 11.6 и далее) дают примерно так: для неуспешного поиска — `1/(1-α) ≈ 3.3` проб, для успешного — около `(1/α)·ln(1/(1-α)) ≈ 1.7` проб в среднем. Это ориентир именно для двойного хеширования; линейное и квадратичное пробирование на практике проигрывают этой оценке из-за кластеризации, описанной выше.

### Общий интерфейс — `IBackend.hpp`

Чисто абстрактный класс с четырьмя методами (`Insert`, `Search`, `Delete`, `Size`, `Capacity`), которые обязаны реализовать оба бэкенда. `HashMap` работает с ним только через указатель, не зная, какая именно стратегия скрыта внутри — классический пример полиморфизма через интерфейс вместо `if/switch` по всему коду.

### Фасад — `HashMap.hpp`

`HashMap<K, V>` ничего не делает сам — это тонкая обёртка над `std::unique_ptr<detail::IBackend<K, V>>`. При создании по значению `Strategy` (`Chaining`, `LinearProbing`, `QuadraticProbing`, `DoubleHashing`) статический метод `MakeBackend` собирает нужную реализацию. Дальше все вызовы — просто делегирование в `backend_`. `LoadFactor()` — единственный метод, который считается прямо здесь, как `Size() / Capacity()`.

### Сложность операций

| Структура | Insert (в среднем) | Search (в среднем) | Delete (в среднем) | Память |
|---|---|---|---|---|
| `DirectAddressTable` | O(1) всегда | O(1) всегда | O(1) всегда | O(\|U\|) |
| `ChainingBackend` | O(1) амортизированно | O(1 + α) | O(1 + α) | O(n + m) |
| `OpenAddressingBackend` (линейное) | O(1/(1-α)), хуже из-за кластеризации | O(1/(1-α)) | O(1/(1-α)) | O(m) |
| `OpenAddressingBackend` (квадратичное) | ближе к O(1/(1-α)) | ближе к O(1/(1-α)) | ближе к O(1/(1-α)) | O(m) |
| `OpenAddressingBackend` (двойное) | ≈ O(1/(1-α)) | ≈ O(1/(1-α)) | ≈ O(1/(1-α)) | O(m) |

где `α` — коэффициент заполнения, `n` — число элементов, `m` — число бакетов/слотов.

### Сборка и тестирование

`stress_test.cpp` не полагается на ручные проверки — он сверяет каждую из четырёх стратегий с `std::unordered_map` как эталоном (`FuzzAgainstOracle`), отдельно для `int` и `string` ключей, вставляя, ища и удаляя случайные ключи в случайной пропорции. Дополнительно есть проверка роста (`GrowthStressTest` — коэффициент заполнения не должен превышать 1.0 после серии вставок, а старые ключи не должны теряться при ресайзе), замер производительности (`TimingBenchmark`) и проверка самой универсальной хеш-функции на равномерность через критерий хи-квадрат (`UniversalHashDistributionTest`) — распределение попаданий по бакетам не должно значимо отклоняться от равномерного.

```bash
cd cmake-build-debug
cmake --build . --target stress_hash_map
./stress_hash_map [operations] [seed]
```

Оба аргумента опциональны: `operations` — число операций на один прогон фаззера (по умолчанию 200000), `seed` — зерно генератора случайных чисел.

### Литература

- Кормен Т., Лейзерсон Ч., Ривест Р., Штайн К. «Алгоритмы: построение и анализ» — главы 11 (хеш-таблицы, прямая адресация, цепочки, открытая адресация, универсальное хеширование).
- Курс «Алгоритмы и структуры данных» ФКН НИУ ВШЭ — материалы по хешированию и амортизационному анализу удвоения таблиц.

---

## English

A hash table is a structure that stores key-value pairs and, on average, gives O(1) insert, search, and delete. The price for that is collisions: with enough keys and few enough slots, two different keys almost inevitably land in the same place. The difference between hash table implementations mostly comes down to how they deal with that.

This project has five structures sharing a common `IBackend` interface, plus a standalone `DirectAddressTable` for the case where collisions aren't a concern at all.

### Direct addressing — `DirectAddressTable.hpp`

The simplest case: the key universe `U` is small enough to just allocate an array of size `|U|`, where `T[k]` stores the value for key `k` directly, no hashing involved. This is classic direct addressing (Cormen, Leiserson, Rivest, Stein, chapter 11.1).

`slots_` is a vector of `std::optional<V>` sized `universe_size`. `Insert`, `Search`, and `Delete` are honest O(1) — a plain index access; `CheckRange` throws if the key falls outside the universe. The cost of that speed is memory: the table always occupies `O(|U|)`, even if only a handful of keys out of a million are actually used. That's the limitation baked in — this isn't a general-purpose structure, it's a tool for cases where keys are, say, small integers or indices with a known upper bound.

### Universal hashing — `UniversalHash.hpp`

The problem with any single fixed hash function is that there always exists some input on which it degenerates into one bucket — an adversary (or just an unlucky dataset) can know the function in advance. Universal hashing fixes this by picking the function randomly out of a family every time the table is created or resized.

The family used here is the standard one:

```
h_{a,b}(x) = ((a·x + b) mod p) mod m
```

where `p` is a prime guaranteed larger than any possible `x` (`2^61 - 1` in the code), `a` is drawn uniformly from `{1, ..., p-1}`, `b` uniformly from `{0, ..., p-1}`, and `m` is the table size. Every call to `Reseed()` (in the constructor and in `Rehash`) redraws `a` and `b` via `std::mt19937_64`.

The key property of this family: for any two distinct `x ≠ y`,

```
Pr[h(x) = h(y)] ≤ 1/m
```

the probability is taken over the random choice of `a` and `b`, not over the input. This is where the rest of the math comes from: no matter how many times the program runs on the same keys, there's no way to reliably force a collision, because the function is different each time.

`KeyHasher<K>` wraps `UniversalHash`: an arbitrary key `K` is first reduced to `uint64_t` via `std::hash<K>`, then passed through the universal hash function. Two separate `KeyHasher` instances (as with `h1_` and `h2_` in open addressing) hash independently, each with its own random `a`, `b`.

### Separate chaining — `ChainingBackend.hpp`

Each bucket is a `std::list<std::pair<K, V>>`. On a collision, the element is simply appended to that bucket's list rather than looking for another spot.

- `Insert`: first checks the load factor `(count_ + 1) / buckets_.size()`; if it has reached `max_load_factor_` (1.0 by default), the table doubles *before* the insertion happens. Then it does a linear scan of the target bucket, either updating an existing key or appending a new node.
- `Search` and `Delete` are a linear scan of the relevant bucket.
- `Grow`: allocates a new bucket vector, reseeds the hasher for the new size (`Rehash`), and moves every element from the old table into the new one — the bucket changes because the hash function itself changed.

The math here is textbook (Cormen, chapter 11.2): under simple uniform hashing, with load factor `α = n/m`, the expected length of a non-empty chain is `O(1 + α)`, which is also the expected time for an unsuccessful search. Since this implementation doubles the table before `α` exceeds 1, average chain length never grows unbounded — the amortized cost of an operation stays `O(1)`, and the resize itself, while `O(n)`, happens progressively less often as the table grows (the standard amortized-doubling argument).

### Open addressing — `OpenAddressingBackend.hpp`

Here collisions are resolved differently: every element lives in a single array, `slots_`, and on a collision a key looks for the next free slot along a predetermined probe sequence. The maximum load factor is `0.7` (`kMaxLoadFactor`), noticeably lower than chaining — open addressing degrades sharply as `α` approaches 1, so extra headroom is needed at all times.

The probe sequence is `⟨h(k,0), h(k,1), ..., h(k,m-1)⟩`, and ideally (under the uniform hashing assumption) it should be an equally likely permutation of all `m` indices for each key. Three strategies are implemented via `Probe(base, step, i)`:

- **Linear probing** — `idx = (base + i) mod m`. Cheapest to compute, but prone to primary clustering: adjacent occupied slots tend to merge into long runs, because any key whose `base` falls inside such a run just extends it further.
- **Quadratic probing** — `idx = (base + i²) mod m`. No primary clustering, but secondary clustering appears instead: two keys sharing the same `base` follow the exact same probe sequence. One subtlety: quadratic probing is only guaranteed to visit all `m` slots for certain values of `m` (e.g. a prime `m` of the form `4k+3`, or specific constant choices); for arbitrary even `m`, as happens here after doubling, some slots can in theory be unreachable from a given `base` in a single pass. `InsertInto` guards against this in practice — if a full pass over `i = 0..m-1` finds no free slot, the table simply doubles again.
- **Double hashing** — `idx = (base + i·step) mod m`, with `step` computed from an independent second hasher, `h2_`. This is the closest of the three to true uniform hashing. There's an important correctness nuance here: for `i·step mod m` to cycle through all `m` residues, `step` must be coprime with `m`. `StepHash` computes `step = h2(key) mod (m - 1) + 1`, so it's guaranteed to land in `[1, m-1]`, but coprimality with `m` isn't explicitly enforced — since `m` is always a power of two in this codebase (all initial and grown capacities are), an even `step` restricts the probe to slots of a single parity. This doesn't break correctness (the algorithm still finds its own key or a free slot within whatever part of the table it can reach, and falls back to `Grow` otherwise), but it reduces effective table coverage for some keys, and in practice shows up as somewhat more frequent resizes than the theoretical bound would predict.

Deletion is implemented via **tombstones** (`Node::deleted = true`) rather than physically clearing the slot. This matters: if a delete simply zeroed the slot, a later search walking the same probe sequence could wrongly conclude the chain ends there and stop early, missing a key that's actually stored further along. That's why `Search` and `Delete` only stop on a true `nullptr` — a slot that was never occupied at all — while tombstoned nodes are skipped but don't terminate the scan. Insertion, conversely, remembers the first tombstone it passes as a candidate for reuse (`first_free`), but keeps scanning to the end first, to make sure the key isn't already present further down the sequence.

`Grow` rebuilds the table at the new size, reseeds both hashers (`h1_`, `h2_`), and reinserts only the live (non-deleted) nodes — tombstones naturally disappear on resize, since every surviving element is inserted into the fresh table from scratch.

At `α = 0.7` under uniform hashing, the classical probe-count bounds (Cormen, theorem 11.6 onward) work out to roughly: `1/(1-α) ≈ 3.3` probes for an unsuccessful search, and about `(1/α)·ln(1/(1-α)) ≈ 1.7` probes for a successful one. That's the reference figure specifically for double hashing; linear and quadratic probing underperform this bound in practice because of the clustering described above.

### The common interface — `IBackend.hpp`

A pure abstract class with four operations (`Insert`, `Search`, `Delete`, `Size`, `Capacity`) that both backends must implement. `HashMap` only ever talks to it through a pointer, with no idea which strategy is hiding underneath — a straightforward case of polymorphism replacing an `if`/`switch` scattered through the codebase.

### The facade — `HashMap.hpp`

`HashMap<K, V>` doesn't do any work itself — it's a thin wrapper around a `std::unique_ptr<detail::IBackend<K, V>>`. Given a `Strategy` value (`Chaining`, `LinearProbing`, `QuadraticProbing`, `DoubleHashing`), the static `MakeBackend` method assembles the right implementation. Every other call just delegates to `backend_`. `LoadFactor()` is the one method actually computed here, as `Size() / Capacity()`.

### Operation complexity

| Structure | Insert (avg) | Search (avg) | Delete (avg) | Memory |
|---|---|---|---|---|
| `DirectAddressTable` | O(1) always | O(1) always | O(1) always | O(\|U\|) |
| `ChainingBackend` | O(1) amortized | O(1 + α) | O(1 + α) | O(n + m) |
| `OpenAddressingBackend` (linear) | O(1/(1-α)), worse due to clustering | O(1/(1-α)) | O(1/(1-α)) | O(m) |
| `OpenAddressingBackend` (quadratic) | closer to O(1/(1-α)) | closer to O(1/(1-α)) | closer to O(1/(1-α)) | O(m) |
| `OpenAddressingBackend` (double) | ≈ O(1/(1-α)) | ≈ O(1/(1-α)) | ≈ O(1/(1-α)) | O(m) |

where `α` is the load factor, `n` the number of stored elements, `m` the number of buckets/slots.

### Build and test

`stress_test.cpp` doesn't rely on manual assertions — it checks all four strategies against `std::unordered_map` as an oracle (`FuzzAgainstOracle`), separately for `int` and `string` keys, mixing random inserts, searches, and deletes. There's also a growth check (`GrowthStressTest` — load factor must never exceed 1.0 after a run of inserts, and old keys must survive resizing), a timing benchmark (`TimingBenchmark`), and a uniformity check on the universal hash function itself using a chi-squared test (`UniversalHashDistributionTest`) — the distribution of hits across buckets shouldn't deviate significantly from uniform.

```bash
cd cmake-build-debug
cmake --build . --target stress_hash_map
./stress_hash_map [operations] [seed]
```

Both arguments are optional: `operations` is the number of ops per fuzzing run (default 200000), `seed` is the random number generator seed.

### References

- Cormen T., Leiserson C., Rivest R., Stein C. *Introduction to Algorithms* — chapter 11 (hash tables, direct addressing, chaining, open addressing, universal hashing).
- HSE FCS "Algorithms and Data Structures" course materials — hashing and amortized analysis of table doubling.

---

<br>

> Хеш-таблица не хранит порядок — она хранит только адрес. Ключ приходит, функция считает число, и элемент оказывается там, где случайность решила его положить, а не там, где ему было бы «правильно» лежать по смыслу. Коллизия — это не ошибка устройства, а его нормальное состояние: два разных пути почти всегда рано или поздно приводят в одну и ту же дверь. Вопрос только в том, что происходит дальше — уступают ли место соседям в очереди или ищут себе следующую свободную дверь сами.
>
> *A hash table doesn't keep order — it only keeps an address. A key comes in, the function computes a number, and the element ends up wherever chance decided to put it, not wherever it would "belong" by meaning. A collision isn't a flaw in the design, it's its normal state: two different paths almost always end up at the same door sooner or later. The only question is what happens next — whether they make room in a line at that door, or go looking for the next free one themselves.*