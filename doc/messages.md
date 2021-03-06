# Message codes
[Home](index.md)

Detailed here are the types of messages which can be sent between client and server.
"Client requests" are sent from a client to the server, and "server commands" and "warnings and errors" are sent from the server to a client.

The message text itself is divided by ASCII control characters; they will be represented below by the following symbols:

Symbol  | ASCII value  | Meaning
------- | ------------ | --------
&laquo; | `0x02` (STX) | Start message
&raquo; | `0x03` (ETX) | End message
&#8226; | `0x1F` (US)  | Value delimiter

#### Client requests
Code | Name                     | Syntax                                                               | Description
---: | ------------------------ | -------------------------------------------------------------------- | -----------
0    | `CL_PING`                | &laquo;0&raquo;                                                      | A ping, to measure latency and reassure the server
1    | `CL_I_AM`                | &laquo;1&#8226;username&raquo;                                       | "My name is `username`"
10   | `CL_LOCATION`            | &laquo;10&#8226;x&#8226;y&raquo;                                     | "I want to move to (`x`,`y`)"
20   | `CL_CANCEL_ACTION`       | &laquo;20&raquo;                                                     | "I don't want to finish my current action"
21   | `CL_CRAFT`               | &laquo;21&#8226;id&raquo;                                            | "I want to craft an item using recipe `id`"
22   | `CL_CONSTRUCT_ITEM`      | &laquo;22&#8226;slot&#8226;x&#8226;y&raquo;                          | "I want to construct the item in inventory slot #`slot`, at location (`x`,`y`)"
23   | `CL_CONSTRUCT`           | &laquo;22&#8226;id&#8226;x&#8226;y&raquo;                            | "I want to construct object `id` at location (`x`,`y`)"
24   | `CL_GATHER`              | &laquo;23&#8226;serial&raquo;                                        | "I want to gather object #`serial`"
25   | `CL_DECONSTRUCT`         | &laquo;23&#8226;serial&raquo;                                        | "I want to deconstruct object #`serial`"
26   | `CL_TRADE`               | &laquo;32&#8226;serial&#8226;slot&raquo;                             | "I want to trade using object #`serial`'s merchant slot #`slot`"
30   | `CL_DROP`                | &laquo;30&#8226;serial&#8226;slot&raquo;                             | "I want to drop object #`serial`'s item #`slot`."<br>A serial of `0` uses the user's inventory"
31   | `CL_SWAP_ITEMS`          | &laquo;31&#8226;serial1&#8226;slot1&#8226;serial2&#8226;slot2&raquo; | "I want to swap object #`serial1`'s item #`slot1` with object #`serial2`'s item #`slot2`"<br>A serial of `0` uses the user's inventory."
32   | `CL_SWAP_ITEMS`          | &laquo;32&#8226;serial&#8226;slot&raquo;                             | "I want to take object #`serial1`'s item #`slot1`"
33   | `CL_SET_MERCHANT_SLOT`   | &laquo;33&#8226;serial&#8226;slot&#8226;ware&#8226;wareQty&#8226;price&#8226;priceQty&raquo; | "I want to set object #`serial`'s merchant slot #`slot` to sell `wareQty` `ware`s for `priceQty` `price`s"
34   | `CL_CLEAR_MERCHANT_SLOT` | &laquo;34&#8226;serial&#8226;slot&raquo;                             | "I want to clear object #`serial`'s merchant slot #`slot`"
35   | `CL_MOUNT`               | &laquo;35&#8226;serial&raquo;                                        | "I want to mount vehicle #`serial`"
36   | `CL_DISMOUNT`            | &laquo;35&#8226;x&#8226;y&raquo;                                     | "I want to dismount my vehicle, to location (`x`,`y`)"
40   | `CL_START_WATCHIMG`      | &laquo;40&#8226;serial&raquo;                                        | "Tell me the details of object #`serial`'s inventory and merchant slots, and alert me of any changes"
41   | `CL_STOP_WATCHING`       | &laquo;41&#8226;serial&raquo;                                        | "Stop alerting me of changes to object #`serial`"
50   | `CL_TARGET`              | &laquo;50&#8226;serial&raquo;                                        | "I am targeting NPC #`serial`"
60   | `CL_SAY`                 | &laquo;60&#8226;message&raquo;                                       | "I want to say '`message`' to everybody"
61   | `CL_WHISPER`             | &laquo;61&#8226;username&#8226;message&raquo;                        | "I want to say '`message`' to user `username`"

#### Server commands                                        
Code | Name                      | Syntax                                                                         | Description
---: | ------------------------- | ------------------------------------------------------------------------------ | -----------
100  | `SV_PING_REPLY`           | &laquo;100&raquo;                                                              | A reply to a ping from a client
101  | `SV_WELCOME`              | &laquo;101&raquo;                                                              | "You have been successfully registered"
110  | `SV_USER_DISCONNECTED`    | &laquo;110&#8226;username&raquo;                                               | "User `username` has disconnected"
111  | `SV_USER_OUT_OF_RANGE`    | &laquo;111&#8226;username&raquo;                                               | "User `username` has moved away from you, and you will stop receiving updates from him"
112  | `SV_OBJECT_OUT_OF_RANGE`  | &laquo;112&#8226;serial&raquo;                                                 | "Object #`serial` has moved away from you, and you will stop receiving updates from it"
122  | `SV_LOCATION`             | &laquo;122&#8226;username&#8226;x&#8226;y&raquo;                               | "User `username` is located at (`x`,`y`)"
123  | `SV_INVENTORY`            | &laquo;123&#8226;slot&#8226;type&#8226;quantity&raquo;                         | "Your inventory slot #`slot` contains a stack of `quantity` `type`s"
124  | `SV_OBJECT`               | &laquo;124&#8226;serial&#8226;x&#8226;y&#8226;type&raquo;                      | "Object #`serial` is located at (`x`,`y`), and is a `type`"
125  | `SV_REMOVE_OBJECT`        | &laquo;125&#8226;serial&raquo;                                                 | "Object #`serial` no longer exists"
126  | `SV_MERCHANT_SLOT`        | &laquo;126&#8226;serial&#8226;slot&#8226;ware&#8226;wareQty&#8226;price&#8226;priceQty&raquo; | "Object #`serial`'s merchant slot #`slot` is selling `wareQty` `ware`s for `priceQty` `price`s"
127  | `SV_NPC_HEALTH`           | &laquo;127&#8226;serial&#8226;health&raquo;                                    | "NPC #`serial` has `health` health"
128  | `SV_OBJECT_LOCATION`      | &laquo;128&#8226;serial&#8226;x&#8226;y&raquo;                                 | "Object #`serial` is now located at (`x`,`y`)"
129  | `SV_TRANSFORM_TIME`       | &laquo;129&#8226;serial&#8226;remaining&raquo;                                 | "Object #`serial` will transform in `remaining` ms"
130  | `SV_ACTION_STARTED`       | &laquo;130&#8226;time&raquo;                                                   | "You have begun an action that will take `t` milliseconds"
131  | `SV_ACTION_FINISHED`      | &laquo;131&raquo;                                                              | "You have completed an action"
132  | `SV_CLASS`                | &laquo;132&#8226;username&#8226;className&raquo;                               | "User `username` is a `className`"
133  | `SV_GEAR`                 | &laquo;133&#8226;username&#8226;slot&#8226;id&raquo;                           | "User `username` is wearing gear `id` in slot #`slot`"
134  | `SV_RECIPES`              | &laquo;134&#8226;quantity&#8226;id1&#8226;id2&#8226;...&raquo;                 | "You know `quantity` recipes: `id1`, `id2`, ..."
135  | `SV_NEW_RECIPES`          | &laquo;135&#8226;quantity&#8226;id1&#8226;id2&#8226;...&raquo;                 | "You now know `quantity` additional recipes: `id1`, `id2`, ..."
150  | `SV_OWNER`                | &laquo;150&#8226;serial&#8226;owner&raquo;                                     | "Object #`serial` is owned by `owner`"
151  | `SV_GATHERING_OBJECT`     | &laquo;151&#8226;serial&raquo;                                                 | "Object #`serial` is being gathered"
152  | `SV_NOT_GATHERING_OBJECT` | &laquo;152&#8226;serial&raquo;                                                 | "Object #`serial` is no longer being gathered"
153  | `SV_LOOTABLE`             | &laquo;153&#8226;serial&raquo;                                                 | "NPC #`serial` has loot available"
154  | `SV_NOT_LOOTABLE`         | &laquo;154&#8226;serial&raquo;                                                 | "NPC #`serial` had loot, but doesn't any more"
155  | `SV_MOUNTED`              | &laquo;155&#8226;serial&#8226;username&raquo;                                  | "User `username` is now driving vehicle #`serial`"
156  | `SV_UNMOUNTED`            | &laquo;156&#8226;serial&#8226;username&raquo;                                  | "User `username` is no longer driving vehicle #`serial`"
157  | `SV_CONSTRUCTION_MATERIALS` | &laquo;157&#8226;serial&#8226;n&#8226;id1&#8226;qty1&#8226;id2&#8226;qty2&#8226;...&raquo; | "Object #`serial` requires `n` types of construction materials: `qty1` `id1`s, `qty2` `id2`s, ..."
160  | `SV_HEALTH`               | &laquo;160&#8226;hp&raquo;                                                     | "You have `hp` health"
161  | `SV_STATS`                | &laquo;161&#8226;maxHealth&#8226;attack&#8226;attackTime&#8226;speed&raquo;    | "Your up-to-date stats are as described"
200  | `SV_SAY`                  | &laquo;200&#8226;username&#8226;message&raquo;                                 | "User `username` said '`message`'"
201  | `SV_WHISPER`              | &laquo;201&#8226;username&#8226;message&raquo;                                 | "User `username` said '`message`' to you"

#### Warnings and errors                                          
Code | Name                         | Syntax                               | Description
---: | ---------------------------- | ------------------------------------ | -----------
900  | `SV_DUPLICATE_USERNAME`      | &laquo;900&raquo;                    | The client has attempted to connect with a username already in use
901  | `SV_INVALID_USERNAME`        | &laquo;901&raquo;                    | The client has attempted to connect with an invalid username
902  | `SV_SERVER_FULL`             | &laquo;902&raquo;                    | There is no room for more clients
903  | `SV_INVALID_USER`            | &laquo;903&raquo;                    | "You tried to interact with a nonexistent user"
910  | `SV_TOO_FAR`                 | &laquo;910&raquo;                    | "You are too far away to perform that action"
911  | `SV_DOESNT_EXIST`            | &laquo;911&raquo;                    | "The object you are trying to use does not exist"
912  | `SV_INVENTORY_FULL`          | &laquo;912&raquo;                    | "You cannot receive an item because your inventory is full"
913  | `SV_NEED_MATERIALS`          | &laquo;913&raquo;                    | "You do not have enough materials to craft that item"
914  | `SV_INVALID_ITEM`            | &laquo;914&raquo;                    | "You tried to craft an item that does not exist"
915  | `SV_CANNOT_CRAFT`            | &laquo;915&raquo;                    | "You tried to craft an item that cannot be crafted"
916  | `SV_ACTION_INTERRUPTED`      | &laquo;916&raquo;                    | "Action interrupted"
917  | `SV_EMPTY_SLOT`              | &laquo;917&raquo;                    | "You tried to manipulate an empty inventory slot"
918  | `SV_INVALID_SLOT`            | &laquo;918&raquo;                    | "You tried to manipulate an inventory slot that does not exist"
919  | `SV_CANNOT_CONSTRUCT`        | &laquo;919&raquo;                    | "You tried to construct an item that is not a structure"
920  | `SV_ITEM_NEEDED`             | &laquo;920&#8226;reqItemClass&raquo; | "You tried to perform an action, without the necessary `reqItemClass`"
921  | `SV_BLOCKED`                 | &laquo;921&raquo;                    | "You tried to perform an action at a location that is blocked"
922  | `SV_NEED_TOOLS`              | &laquo;922&raquo;                    | "You tried to craft an item, but need additional tools"
923  | `SV_CANNOT_DECONSTRUCT`      | &laquo;923&raquo;                    | "You tried to deconstruct an object that cannot be deconstructed"
924  | `SV_NO_PERMISSION`           | &laquo;924&raquo;                    | "You don't have permission to do that"
925  | `SV_NOT_MERCHANT`            | &laquo;925&raquo;                    | "You tried to use a non-merchant object as a merchant"
926  | `SV_INVALID_MERCHANT_SLOT`   | &laquo;926&raquo;                    | "You tried to use a merchant object's invalid merchant slot"
927  | `SV_NO_WARE`                 | &laquo;927&raquo;                    | "You tried to purchase from a merchant object, but it doesn't have enough wares in stock"
928  | `SV_NO_PRICE`                | &laquo;928&raquo;                    | "You tried to purchase from a merchant object, but you can't afford it"
929  | `SV_MERCHANT_INVENTORY_FULL` | &laquo;929&raquo;                    | "You tried to purchase from a merchant object, but it doesn't have enough inventory space to accept your payment"
930  | `SV_NOT_EMPTY`               | &laquo;930&raquo;                    | "You tried to deconstruct an object that contains an inventory of items"
931  | `SV_NOT_NPC`                 | &laquo;931&raquo;                    | "You tried to perform an action on an NPC, but that object isn't one"
932  | `SV_NPC_DEAD`                | &laquo;932&raquo;                    | "You tried to perform an action on an NPC, but it's dead"
933  | `SV_NPC_SWAP`                | &laquo;933&raquo;                    | "You tried to put an item into an NPC"
934  | `SV_TAKE_SELF`               | &laquo;934&raquo;                    | "You tried to take an item from yourself"
935  | `SV_NOT_GEAR`                | &laquo;935&raquo;                    | "You tried to equip an item in an incompatible gear slot"
936  | `SV_NOT_VEHICLE              | &laquo;936&raquo;                    | "You tried to enter an object that isn't a vehicle"
937  | `SV_VEHICLE_OCCUPIED         | &laquo;937&raquo;                    | "That action requires the vehicle to be empty"
938  | `SV_NO_VEHICLE               | &laquo;938&raquo;                    | "You tried to dismount while not driving a vehicle"
939  | `SV_UNKNOWN_RECIPE           | &laquo;939&raquo;                    | "You tried to craft using a recipe you don't know"
940  | `SV_UNKNOWN_CONSTRUCTION     | &laquo;940&raquo;                    | "You tried to construct an object without knowing how"
941  | `SV_WRONG_MATERIAL           | &laquo;941&raquo;                    | "You tried to add the wrong item to a construction site"
942  | `SV_UNDER_CONSTRUCTION       | &laquo;942&raquo;                    | "That object is still under construction"
