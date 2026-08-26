# Рабочий контракт проекта IX-Ray

Этот файл содержит только правила, которые нужны почти в каждой задаче.
Подробные регламенты хранятся в [`docs`](docs) и становятся обязательными для
соответствующего вида работ.

## Перед первой записью

1. Прочитать родительский `C:\GPT_Project\Stalker\AGENTS.md`, этот файл и
   [`docs/REPOSITORY_LAYOUT.md`](docs/REPOSITORY_LAYOUT.md).
2. Проверить фактические пути через `Resolve-Path`/`Test-Path`; старые пути из
   чата или скрипта не считать актуальными без проверки.
3. Посмотреть `git status` и не смешивать с задачей чужие или ранее начатые
   изменения.
4. Найти одноимённые копии изменяемого игрового файла в `gamedata` и
   `ixr_addons`, определить фактически активный слой и после правки проверить
   перекрытия повторно.
5. Открыть тематические документы из таблицы ниже до изменения файлов.

Единственный активный продукт и запускаемый проект:

`C:\GPT_Project\Stalker\Chronicles of Pripyat 4.26 IX-Ray`

Готовый игровой результат записывается только сюда. Runtime, исходники движка,
временные сборки, исследования, restore points и корзина не являются частью
продукта или Git; их карта находится в
[`docs/REPOSITORY_LAYOUT.md`](docs/REPOSITORY_LAYOUT.md).

### Где находится канонический `main`

Ветка `main` определяется Git-ссылкой, а не именем папки. Её фактический
worktree всегда находить через `git worktree list`. Текущий чистый
integration-worktree расположен здесь:

`C:\GPT_Project\Stalker\.ixray-local\workspace\pf-irc-presence-main-integration`

Запускаемая папка продукта может оставаться на старой `dev/*`-ветке и быть
грязной из-за совместной установки результатов нескольких задач. Это не
означает, что задача отсутствует в `main`. Перед таким выводом проверить commit
через `git merge-base --is-ancestor <commit> main`, сравнить ветки командой
`git rev-list --left-right --count main...<ветка>` и сверить только файлы своей
задачи с `main`.

Запрещено ради «очистки ветки» выполнять в продуктовой папке `switch`, `reset`,
`stash`, `clean` или удалять чужие файлы. Чистота обязательна для выделенного
integration-worktree `main`; продуктовая папка является общей установленной
копией для запуска и проверки.

## Как работаем с движком

Движковые исходники, сборочные файлы, `xrEngine.exe` и DLL запрещено изменять
или пересобирать без прямого разрешения пользователя на конкретную движковую
правку. До разрешённой работы полностью прочитать
[`docs/ENGINE_CHANGE_WORKFLOW.md`](docs/ENGINE_CHANGE_WORKFLOW.md) и сверить
все активные записи
[`docs/ENGINE_PATCH_LEDGER.md`](docs/ENGINE_PATCH_LEDGER.md).

Обязательный короткий контракт:

1. База — только установленный в `bin` комплект последнего подтверждённого
   запуска, сопоставленный с исходниками по SHA-256 и Git commit. Дата каталога
   или «самый новый build» не являются доказательством.
2. Исходники и чистая сборка живут только во внешнем `.ixray-local/workspace`;
   build-каталог является кандидатом, а не актуальным продуктом.
3. Каждая самостоятельная правка получает ID `COP-ENG-NNN` или
   `COP-BUILD-NNN`, inline-маркер `COP_ENGINE_PATCH: <ID>`, отдельный commit и
   полную запись в реестре.
4. До установки обязательны сверка всего patchset, проверенный restore point,
   SHA-256 кандидата и закрытый `xrEngine.exe`. Старую DLL нельзя ставить ради
   возврата одной функции: потерянный патч переносится в канонический source.
5. После установки выполнить полный запуск целевого проекта, проверить функцию
   и свежий runtime-лог, затем записать установленный хеш, commit, restore point
   и журнал в реестр и `docs/CHANGELOG.md`.

Если решение требует движка, но прямого разрешения нет, зафиксировать
`ТРЕБУЕТ РЕШЕНИЯ` в
[`docs/IXRAY_UPDATE_ADAPTATION.md`](docs/IXRAY_UPDATE_ADAPTATION.md), сообщить
блокер и не изменять бинарники или source.

## Как фиксируем и интегрируем правки

`main` — единственная каноническая линия завершённого продукта. `dev/<задача>`
является временной рабочей веткой, а `test/<задача>` создаётся только по прямому
запросу на изолированный эксперимент. Внешняя сборка не становится актуальной,
пока проверенный результат не установлен в целевой проект и не связан с
каноническим commit.

Если пользователь прямо не потребовал изоляцию, завершение обычной задачи
включает:

1. выполнить профильные проверки и просмотреть итоговый diff;
2. закоммитить только файлы этой задачи, не пряча и не присваивая чужие правки;
3. учесть актуальный `main`, интегрировать завершённую ветку и повторить
   необходимые проверки общего результата;
4. проверить чистый integration-worktree ветки `main` и выполнить в нём
   read-only аудит; переключать общую запускаемую папку продукта на `main` не
   требуется и при чужих незакоммиченных изменениях запрещено:

   ```powershell
   powershell -ExecutionPolicy Bypass -File tools\audit_product_state.ps1 `
     -RequireCanonicalClean
   ```

5. для EXE/DLL дополнительно подтвердить SHA-256 установленного файла и запись
   в реестре движковых патчей.

Если интеграции мешают чужие незакоммиченные изменения, активный процесс или
неясное происхождение файлов, ничего не stash/перекладывать автоматически:
сохранить состояние, выполнить read-only аудит и сообщить конкретный блокер.
Отправка во внешний remote или создание PR выполняется только по прямому
запросу пользователя. Полный порядок — в
[`docs/GIT_PRODUCT_WORKFLOW.md`](docs/GIT_PRODUCT_WORKFLOW.md).

## Маршрутизация по документации

| Вид работы | Что обязательно прочитать |
|---|---|
| Рабочие пути, доноры, перекрытия, QA, общие исключения | [`REPOSITORY_LAYOUT.md`](docs/REPOSITORY_LAYOUT.md), [`PROJECT_WORK_RULES.md`](docs/PROJECT_WORK_RULES.md) |
| Runtime, launcher и пользовательский релиз | [`RUNTIME_AND_RELEASE_LAYOUT.md`](docs/RUNTIME_AND_RELEASE_LAYOUT.md) |
| Restore point или временное удаление | [`RESTORE_POINTS_RULES.md`](docs/RESTORE_POINTS_RULES.md), [`PROJECT_WORK_RULES.md`](docs/PROJECT_WORK_RULES.md) |
| Обновление IX-Ray или несовместимость | [`IXRAY_UPDATE_ADAPTATION.md`](docs/IXRAY_UPDATE_ADAPTATION.md) |
| Lua-модули и горячие callbacks | [`LUA_SCRIPT_ARCHITECTURE.md`](docs/LUA_SCRIPT_ARCHITECTURE.md), [`PERFORMANCE_AND_LONG_SESSION.md`](docs/PERFORMANCE_AND_LONG_SESSION.md) |
| Порт, физика, звук или интерактивные части автомобилей DCP | [`DCP_VEHICLE_ENGINE_REFERENCE.md`](docs/DCP_VEHICLE_ENGINE_REFERENCE.md), [`DCP_VEHICLE_PORTING_WORKFLOW.md`](docs/DCP_VEHICLE_PORTING_WORKFLOW.md), а при движковой правке также [`ENGINE_CHANGE_WORKFLOW.md`](docs/ENGINE_CHANGE_WORKFLOW.md) и [`ENGINE_PATCH_LEDGER.md`](docs/ENGINE_PATCH_LEDGER.md) |
| Новый, удалённый или изменённый предмет | [`INVENTORY_GROUPING_RULES.md`](docs/INVENTORY_GROUPING_RULES.md) |
| Постоянные двери и лампы | [`PF_STATIC_DOOR_WORKFLOW.md`](docs/PF_STATIC_DOOR_WORKFLOW.md), [`PF_STATIC_LAMP_WORKFLOW.md`](docs/PF_STATIC_LAMP_WORKFLOW.md), [`PF_SOUND_LAMP_WORKFLOW.md`](docs/PF_SOUND_LAMP_WORKFLOW.md), [`PF_RED_SOUND_LAMP_WORKFLOW.md`](docs/PF_RED_SOUND_LAMP_WORKFLOW.md) |
| Постоянные/статические и квестовые NPC, спавн, logic и анимации | Каноническая инструкция статической позы: [`NPC_SCENARIO_POSE_WORKFLOW.md`](docs/NPC_SCENARIO_POSE_WORKFLOW.md); также [`QUEST_NPC_PORTING_RULES.md`](docs/QUEST_NPC_PORTING_RULES.md), [`PF_BUNKER_NPC_RUNTIME.md`](docs/PF_BUNKER_NPC_RUNTIME.md), [`IXRAY_NPC_SPAWN_DEBUG_NOTES.md`](docs/IXRAY_NPC_SPAWN_DEBUG_NOTES.md) |
| Главное и пауза-меню, настройки, загрузка и сохранение | [`MENU_UI_CANON.md`](docs/MENU_UI_CANON.md), [`PROJECT_WORK_RULES.md`](docs/PROJECT_WORK_RULES.md) |
| КПК, DDS, энциклопедия и тултипы | [`PDA_UX_UI_RULES.md`](docs/PDA_UX_UI_RULES.md), [`PROJECT_WORK_RULES.md`](docs/PROJECT_WORK_RULES.md) |
| Любая работа с тайниками, координатами, лутом, КПК, взломом/перепрошивкой, USB, наводками, ЧБ-металлоискателями, странными расходниками или stash-QA | Сначала полностью [`STASH_SEARCH_SYSTEM_CONTRACT.md`](docs/STASH_SEARCH_SYSTEM_CONTRACT.md), затем [`STASH_RULES.md`](docs/STASH_RULES.md), [`STASH_ECONOMY_BALANCE.md`](docs/STASH_ECONOMY_BALANCE.md); для Lua также [`LUA_SCRIPT_ARCHITECTURE.md`](docs/LUA_SCRIPT_ARCHITECTURE.md) и [`PERFORMANCE_AND_LONG_SESSION.md`](docs/PERFORMANCE_AND_LONG_SESSION.md) |
| Будущая варка артефактов | [`ARTIFACT_COOKING_CONCEPT.md`](docs/ARTIFACT_COOKING_CONCEPT.md) |

Полный индекс находится в [`docs/README.md`](docs/README.md). В `AGENTS.md`
не возвращать длинные технические расследования, координаты, частные параметры
объектов и журналы конкретных исправлений: они должны жить в тематическом
документе.

## Общие ворота завершения

- Проверить фактически активные копии в `ixr_addons`.
- Запустить валидаторы, относящиеся к изменённым типам файлов.
- Для Lua использовать совместимый синтаксический анализатор 5.1.
- После изменения кэшируемых Lua/UI/ресурсов полностью перезапустить целевой
  `xrEngine.exe`; простая загрузка сохранения недостаточна.
- Отдельно проверить новую игру и существующее сохранение, если изменение
  затрагивает runtime-состояние. Статический `all.spawn` по умолчанию требует
  новой игры и не получает миграцию без прямого запроса.
- Совместимость с будущим IX-Ray зафиксировать в
  `docs/IXRAY_UPDATE_ADAPTATION.md` в рамках той же задачи.
