#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos.config.h
 * Switches to define the most relevant compile-time settings of RTuinoOS in an application
 * specific way.
 *
 * Copyright (C) 2012 Peter Vranken (mailto:Peter_Vranken@Yahoo.de)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Include files
 */


/*
 * Defines
 */

/** Does the task scheduling concept support time slices of limited length for activated
    tasks? If on, the overhead of the scheduler slightly increases.\n
      Select either RTOS_FEATURE_OFF or RTOS_FEATURE_ON. */
#define RTOS_ROUND_ROBIN_MODE_SUPPORTED     RTOS_FEATURE_OFF


/** Number of tasks in the system. Tasks aren't created dynamically. This number of tasks
    will always be existent and alive. Permitted range is 0..255.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_NO_TASKS    1


/** Number of distinct priorities of tasks. Since several tasks may share the same
    priority, this number is lower or equal to NO_TASKS. Permitted range is 0..NO_TASKS,
    but 1..NO_TASKS if at least one task is defined.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_NO_PRIO_CLASSES 1


/** Since many tasks will belong to distinct priority classes, the maximum number of tasks
    belonging to the same class will be significantly lower than the number of tasks. This
    setting is used to reduce the required memory size for the statically allocated data
    structures. Set the value as low as possible. Permitted range is min(1, NO_TASKS)..255,
    but a value greater than NO_TASKS is not reasonable.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_MAX_NO_TASKS_IN_PRIO_CLASS 1


/*
 * Global type definitions
 */


/*
 * Global data declarations
 */


/*
 * Global prototypes
 */



#endif  /* RTOS_CONFIG_INCLUDED */
