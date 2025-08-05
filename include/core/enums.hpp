#pragma once

enum class FLAG { CARRY = 4, HALF_CARRY = 5, NEGATIVE = 6, ZERO = 7 };
enum class STATUS { ACTIVE, HALT_MODE, STOP, PAUSED };
enum class SPEED { NORMAL = 0x00, DOUBLE = 0x80 };