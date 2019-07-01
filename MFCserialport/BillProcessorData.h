#pragma once

#pragma warning( disable : 4200 )
#pragma warning( disable : 4819 )

  #ifdef BNRDLL_EXPORTS
    #define BNRDLL_API __declspec(dllexport)
  #else
    #define BNRDLL_API __declspec(dllimport)
  #endif

#ifdef BNRCTLJNA
  #define CALLTYPE
#else
  #define CALLTYPE __stdcall
#endif

//BNRDLL_API T_BnrXfsResult CALLTYPE module_GetStatus(T_ModuleId id, T_ModuleStatus * moduleStatus);
typedef UINT32 T_ModuleId;

//#define __nullterminated         __allowed(on_typedecl)
#define IN
#define FALSE   0
#define TRUE    1
#define NULL    0
#define SIZE_OF_PHYSICAL_NAME (5)
#define _T(x)       __T(x)
#define _In_                            _SAL2_Source_(_In_, (), _Pre1_impl_(__notnull_impl_notref) _Pre_valid_impl_ _Deref_pre1_impl_(__readaccess_impl_notref))
#define _In_opt_                        _SAL2_Source_(_In_opt_, (), _Pre1_impl_(__maybenull_impl_notref) _Pre_valid_impl_ _Deref_pre_readonly_)
#define NBMAXELEMENTS                 (20)  /**< Maximum of elements */
#ifndef CONST
#define CONST               const
#endif


typedef unsigned long ULONG_PTR;
typedef ULONG_PTR SIZE_T;
typedef int                 INT;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef signed int          INT32;
typedef unsigned int        UINT32;
typedef char TCHAR;
typedef char CHAR;
typedef __nullterminated CONST CHAR *LPCSTR;
typedef LPCSTR PCTSTR, LPCTSTR, PCUTSTR, LPCUTSTR;

typedef enum OperationalStatus {
  OS_OPERATIONAL,           /**< The element is operational - No errors. */
  OS_CHECKING_ERROR,        /**< Can not determine operational state of the element - Will be determinated on next test. */
  OS_NOT_OPERATIONAL,       /**< Element not operational - Element or sub-element part has an error. */
  OS_OPERATIONAL_DEGRADED,  /**< For future use. */
  OS_SW_LOCKED              /**< The module is locked. Use module_ConfigureCashUnit() to unlock the corresponding PhysicalCashUnit. */
} T_OperationalStatus;

typedef struct ModuleStatusBaseClass {
  T_ModuleId           id;                 /**< Id of the MainModule module. @see Module Identification. */
  T_OperationalStatus  operationalStatus;  /**< Operational status of the module. Possible values are the same for all operational statuses. @see T_OperationalStatus. */
} T_ModuleStatusBaseClass;

typedef enum MainModuleErrorCode {
  MMEC_NO_ERROR =        0,  /**< No error. */
  MMEC_INCOMPATIBLE_SW = 1,  /**< Software not compatible with one of the Main Module component. */
  MMEC_OPENED =          2,  /**< Module is open. */
  MMEC_BOOT_RUNNING =    3   /**< The boot application is running. There may be 2 reasons : - mmApplication is not available. - mmMainBoot has to be tested after a new firmware update. Just reboot again the BNR to start mmApplication. */
} T_MainModuleErrorCode;

typedef enum BillTransportStatus {
  BTS_OK,              /**< No error and no bill is stopped in the transport system. */
  BTS_BILL_STOP,       /**< One or more bills are stopped in the transport system due to a power down, system restart, system opening or a hardware failure (e.g.\ motor). */
  BTS_BILL_JAM,        /**< One or more bills are stopped in the transport system due to a bill jam or an undetected failure. */
  BTS_BILL_ERROR,      /**< An non transportable bill or bundle has been detected in the transport system.*/
  BTS_TRANSPORT_ERROR  /**< Something is preventing the motor from spinning or the coding wheel is unable to sense that the motor is spinning.  This could be a faulty motor, a faulty sensor/broken coding wheel, a faulty board, or in rare cases a jam.*/
} T_BillTransportStatus;

typedef enum BillTransportSection {
  BTSE_UNKNOWN                = 0,  /**< The section is not available. */
  BTSE_INLET_FW               = 1,  /**< In the inlet section while moving forward.*/
  BTSE_INLET_BW               = 2,  /**< In the inlet section while moving backward.*/
  BTSE_POSITIONER_FW          = 3,  /**< In the positioner section while moving forward. */
  BTSE_POSITIONER_BW          = 4,  /**< In the positioner section moving backward. */
  BTSE_RECOGNITION_SENSOR_FW  = 5,  /**< In the recognition sensor section while moving forward. */
  BTSE_RECOGNITION_SENSOR_BW  = 6,  /**< In the recognition sensor section moving backward. */
  BTSE_RS_SP_INTERFACE_FW     = 7,  /**< Between recognition sensor (RS) and spine (SP) sections while moving forward. */
  BTSE_RS_SP_INTERFACE_BW     = 8,  /**< Between recognition sensor (RS) and spine (SP) sections moving backward. */
  BTSE_SPINE_FW               = 9,  /**< In the spine section while moving forward. */
  BTSE_SPINE_BW               = 10, /**< In the spine section moving backward. */
  BTSE_MODULE_INTERFACE_FW    = 11, /**< Between a module and the spine sections while moving forward. */
  BTSE_MODULE_INTERFACE_BW    = 12, /**< Between a module and the spine sections moving backward. */
  BTSE_MODULE_FW              = 13, /**< In the module section while moving forward. */
  BTSE_MODULE_BW              = 14, /**< In the module section moving backward. */
  BTSE_BT_SP_INTERFACE_FW     = 15, /**< Between the MainModule bottom transport and the spine sections while moving forward. */
  BTSE_BT_SP_INTERFACE_BW     = 16, /**< Between the MainModule bottom transport and the spine sections moving backward. */
  BTSE_BOTTOM_TRANSPORT_FW    = 17, /**< In the MainModule bottom transport section while moving forward. */
  BTSE_BOTTOM_TRANSPORT_BW    = 18, /**< In the MainModule bottom transport section moving backward. */
  BTSE_BUNDLER_FW             = 19, /**< In the bundler section while moving forward. */
  BTSE_BUNDLER_BW             = 20, /**< In the bundler section moving backward. */
  BTSE_OUTLET_FW              = 21, /**< In the outlet section while moving forward. */
  BTSE_OUTLET_BW              = 22, /**< In the outlet section moving backward. */
  BTSE_SPINE_EXIT_FW          = 25, /**< In the Spine exit section, in forward direction (BNA6F only).*/
  BTSE_SPINE_EXIT_BW          = 26  /**< In the Spine exit section, in backward direction (BNA6F only).*/
} T_BillTransportSection;

typedef struct ElementStatus {
  T_ModuleId           id;
  T_OperationalStatus  operationalStatus;
} T_ElementStatus;

typedef enum SensorErrorCode {
  DEC_NO_ERROR =       0,  /**< No error. */
  DEC_SIGNAL_HIGH =    1,  /**< Detector could not be regulated because signal is too high. */
  DEC_SIGNAL_LOW =     2,  /**< Detector could not be regulated because signal is too low. */
  DEC_COM_BREAKDOWN =  3,  /**< No communication with the module that owns the sensor. */
  DEC_BAD_REGULATION = 4,  /**< Transitory error code, sensor regulation attempts are made (bundler's sensor only). */
} T_SensorErrorCode;


typedef enum SensorFunctionalStatus {
  DFS_UNKNOWN =           0,  /**< The status of the sensor can not be determined. */
  DFS_UNCOVERED =         1,  /**< The sensor is uncovered. */
  DFS_COVERED =           2,  /**< The sensor is covered. */
  DFS_PARTIALLY_COVERED = 3,  /**< The sensor is partially covered (BI.S1 and BI.S3 only). */
  DFS_DETECT_1 =          4,  /**< For internal use only. */
  DFS_DETECT_2 =          5  /**< For internal use only. */
} T_SensorFunctionalStatus;

typedef struct SensorStatus {
  T_ModuleId                id;
  T_OperationalStatus       operationalStatus;
  T_SensorFunctionalStatus  functionalStatus;
  T_SensorErrorCode         errorCode;
} T_SensorStatus;

typedef enum CoverFunctionalStatus {
  CFS_UNKNOWN,  /**< The status of the cover can not be determined. */
  CFS_OPENED,   /**< The cover is opened. */
  CFS_CLOSED    /**< The cover is closed. */
} T_CoverFunctionalStatus;

typedef struct CoverStatus {
  T_ModuleId               id;
  T_OperationalStatus      operationalStatus;  /**< Always set to OS_OPERATIONAL. */
  T_CoverFunctionalStatus  functionalStatus;
} T_CoverStatus;

typedef enum ModuleErrorCode {
  MODEC_NO_ERROR,               /**< No error. */
  MODEC_COM_BREAKDOWN,          /**< No communication with the module. */
  MODEC_INCOMPATIBLE_MODULE,    /**< The firmware of this module is incompatible with the Main Module. */
  MODEC_MISSING_MODULE,         /**< No module present in the system. */
  MODEC_OPENED,                 /**< The module is open; for Spine module only. */
  MODEC_WRONG_MODULE_TYPE,      /**< The type of the module is not compatible with the BNR configuration. */
  MODEC_BOOT_RUNNING,           /**< Module working with the boot application. */
  MODEC_CANNOT_EXTRACT_BILL,    /**< @deprecated No more used. */
  MODEC_INCOMPATIBLE_SOFTWARE,  /**< @deprecated No more used. */
  MODEC_MODULE_NOT_DETECTED,    /**< The module has been removed during an exchange operation. Can happen only when the cash module lock is open; for Cashbox module only. */
  MODEC_SHUTTER_CLOSED,         /**< The Cashbox shutter is closed and cannot be opened by the BNR; for Cashbox module only. */
  MODEC_NO_COM,                 /**< A smartCashbox is configured but no communication is detected when the cash modules lock is open; for Cashbox module only.*/
  MODEC_SIGNAL_TOO_HIGH,        /**< Sensor signal is too high; for BarcodeReader module only.*/
  MODEC_SIGNAL_TOO_LOW          /**< Sensor signal is too low; for BarcodeReader module only.*/
} T_ModuleErrorCode;

typedef enum MotorFunctionalStatus {
  MFS_UNKNOWN,
  MFS_STOPPED,   
  MFS_RUNNING
} T_MotorFunctionalStatus;

/** Motor Error Code. */
typedef enum MotorErrorCode {
  MEC_NO_ERROR =      0,  /**< No error. */
  MEC_COM_BREAKDOWN = 1,  /**< No communication with the module that owns the motor. */
  MEC_NOT_STARTED =   2,  /**< Motor failed to start. */
  MEC_SPEED_TO_LOW =  3   /**< Motor speed low limit has been reached. */
} T_MotorErrorCode;

typedef struct MotorStatus {
  T_ModuleId               id;
  T_OperationalStatus      operationalStatus;
  T_MotorFunctionalStatus  functionalStatus;
  T_MotorErrorCode         errorCode;
} T_MotorStatus;

typedef enum StackerFunctionalStatus {
  SFS_UNKNOWN =           0,
  SFS_HOME_POSITION =     1,
  SFS_TRASHBIN_POSITION = 2,
} T_StackerFunctionalStatus;

typedef enum StackerErrorCode {
  STEC_NO_ERROR =                0,  /**< No error. */
  STEC_CANNOT_POSITION =         1,  /**< ST.S2 sensor did not change while moving around home position. */
  STEC_CANNOT_MOVE_TO_TRASHBIN = 2,  /**< The stacker motor was blocked while trying to move to trashbin position from its home position. The stacker is still in home position. */
  STEC_HARDWARE_FAILURE =        3,  /**< The stacker sensor ST.S2 not covered after some steps. */
  STEC_BLOCKED_AT_HOME =         4,  /**< The stacker motor did not start. The stacker is still in home position. */
  STEC_BLOCKED_AT_BILL_PATH =    5,  /**< While moving up, the stacker was blocked at the bill path position. */
  STEC_BLOCKED_AT_CB_ENTRY =     6,  /**< While moving up, the stacker was blocked at the cashbox entry. */
  STEC_BLOCKED_AROUND_HOME =     7,  /**< While moving up, the stacker was blocked around its home position. */
  STEC_BLOCKED_IN_CB =           8,  /**< While moving up, the stacker was blocked inside the cashbox. */
  STEC_STOPPED_AT_BILL_PATH =    9,  /**< While moving down, the stacker was stopped at the bill path position. BNR tries to move back the stacker in home position before declaring this error. */
  STEC_STOPPED_AT_CB_ENTRY =    10,  /**< While moving down, the stacker was stopped at the cashbox entry. BNR tries to move back the stacker in home position before declaring this error.May be due to bad insertion of the cashbox. */
  STEC_STOPPED_IN_CB =          11,  /**< While moving down, the stacker was stopped inside the cashbox. BNR tries to move back the stacker in home position before declaring this error. Plate might be blocked. */
  STEC_NO_CYCLE_ALLOWED =       12,  /**< No cycle is allowed (bundle/bill present in bottomTransport area). */
} T_StackerErrorCode;

typedef struct StackerStatus {
  T_ModuleId                 id;
  T_OperationalStatus        operationalStatus;
  T_StackerFunctionalStatus  functionalStatus;
  T_StackerErrorCode         errorCode;
} T_StackerStatus;

typedef enum BundlerDivSystemFunctionalStatus {
  BDFS_UNKNOWN =             0,
  BDFS_BUNDLER_N_POSITION =  1,
  BDFS_OUTLET_POSITION =     2,
  BDFS_TRASHBIN_POSITION =   3,
  BDFS_EXTRACTION_POSITION = 4,
} T_BundlerDivSystemFunctionalStatus;

typedef enum BundlerDivSystemErrorCode {
  BDEC_NO_ERROR =                     0,  /**< No error. */
  BDEC_BLOCKED =                      1,  /**< The bundler div system motor speed was null or too low. */
  BDEC_CANNOT_POSITION =              2,  /**< The bundler div system failed to reach te requested position. */
  BDEC_EXTRACTION_POS_NOT_CONFIRMED = 3,  /**< The bundler div system position confirmation sensor (BU.S3) should have been covered on extraction position. */
  BDEC_OUTLET_POS_NOT_CONFIRMED =     4,  /**< The bundler div system position confirmation sensor (BU.S3) should have been covered on outlet position. */
  BDEC_CANNOT_FIND_MARK =             5,  /**< The bundler div system could not find the position mark during initialization cycle. */
} T_BundlerDivSystemErrorCode;

typedef struct BundlerDivSystemStatus {
  T_ModuleId                          id;
  T_OperationalStatus                 operationalStatus;
  T_BundlerDivSystemFunctionalStatus  functionalStatus;
  T_BundlerDivSystemErrorCode         errorCode;
} T_BundlerDivSystemStatus;

typedef enum RecognitionSensorErrorCode {
  RSEC_NO_ERROR,                       /**< No error. */
  RSEC_OFFSET_COMPENSATION_ERROR,      /**< Impossible to compensate the offset */
  RSEC_REMAINING_OFFSET_TOO_LOW,       /**< The remaining offset is too low. */
  RSEC_REMAINING_OFFSET_TOO_HIGH,      /**< The remaining offset is too high. */
  RSEC_CANNOT_REGULATE_FOR_DSR_IDENT,  /**< Impossible to regulate for identification. */
  RSEC_C_COEF_DIV_BY_ZERO,             /**< Divide by zero while calculating C coefficients. */
  RSEC_C_COEF_NEG_VALUE,               /**< One or more C coefficients are below zero. */
  RSEC_C_COEF_OVERFLOW,                /**< Overflow on C coefficients. */
  RSEC_C_COEF_OUT_OF_RANGE,            /**< One or more C coefficient values are out of range. */
  RSEC_CANNOT_REGULATE                 /**< Impossible to regulate the sensor. */
} T_RecognitionSensorErrorCode ;


typedef struct RecognitionSensorStatus {
  T_ModuleId                    id;
  T_OperationalStatus           operationalStatus;
  T_RecognitionSensorErrorCode  errorCode;
  T_SensorFunctionalStatus      upFunctionalStatus;    /**< Functional Status of the RS Bill Sensor Up */
  T_SensorFunctionalStatus      downFunctionalStatus;  /**< Functional Status of the RS Bill Sensor Down */
} T_RecognitionSensorStatus;

typedef enum PositionerFunctionalStatus {
  POS_UNKNOWN,
  POS_POSITIONING,
  POS_TRANSPORT,
} T_PositionerFunctionalStatus;

typedef enum PositionerErrorCode {
  PEC_NO_ERROR,                   /**< No error. */
  PEC_CANNOT_MOVE_TO_POSITIONING  /**< Cannot move to positioning position. */
} T_PositionerErrorCode;

typedef struct PositionerStatus {
  T_ModuleId                    id;
  T_OperationalStatus           operationalStatus;
  T_PositionerFunctionalStatus  functionalStatus;
  T_PositionerErrorCode         errorCode;
} T_PositionerStatus;

typedef enum CashboxStackHeightErrorCode {
  CSHEC_NO_ERROR,
  CSHEC_SENSOR_STAYS_COVERED,
} T_CashboxStackHeightErrorCode;

typedef enum CashboxStackHeightFunctionalStatus {
  CSHFS_UNKNOWN,
  CSHFS_OK,
  CSHFS_LAST_BUNDLE,
  CSHFS_MAX_HEIGHT,
  CSHFS_HIGH
} T_CashboxStackHeightFunctionalStatus;


typedef struct CashboxStackHeightStatus {
  T_ModuleId                            id;
  T_OperationalStatus                   operationalStatus;  /**< @deprecated No more used, always set to #OS_OPERATIONAL (2010-05-10). */
  T_CashboxStackHeightErrorCode         errorCode;          /**< @deprecated No more used, always set to #CSHEC_NO_ERROR (2011-06-17).*/
  T_CashboxStackHeightFunctionalStatus  functionalStatus;
} T_CashboxStackHeightStatus;

typedef enum FlapErrorCode {
  FEC_NO_ERROR,       /**< No error. */
  FEC_STAYS_CLOSED,   /**< The flap stays closed. */
  FEC_COM_BREAKDOWN,  /**< No communication with the flap. */
  FEC_STAYS_OPENED    /**< The flap stays opened. */
} T_FlapErrorCode;

typedef enum FlapFunctionalStatus {
  FFS_UNKNOWN,  /**< The status can not be determined. */
  FFS_CLOSED,   /**< The flap is closed. */
  FFS_OPENED    /**< The flap is openned. */
} T_FlapFunctionalStatus;

typedef struct FlapStatus {
  T_ModuleId              id;                 /**< Id of the module. */
  T_OperationalStatus     operationalStatus;  /**< Operational status of the flap. */
  T_FlapErrorCode         errorCode;          /**< Error code of the element. Useful if the operationalStatus is different from #OS_OPERATIONAL */
  T_FlapFunctionalStatus  functionalStatus;   /**< Functional status of the flap. */
} T_FlapStatus;

typedef union MainModuleElementStatusItem {
  T_ElementStatus             elementStatus;
  T_SensorStatus              sensorStatus;
  T_CoverStatus               coverStatus;
  T_MotorStatus               motorStatus;
  T_StackerStatus             stackerStatus;
  T_BundlerDivSystemStatus    bundlerDivSystemStatus;
  T_RecognitionSensorStatus   recognitionSensorStatus;
  T_PositionerStatus          positionerStatus;
  T_CashboxStackHeightStatus  cashboxStackHeightStatus;
  T_FlapStatus                outletFlapStatus;
  T_SensorStatus              detectorStatus;            /**< @deprecated Use #T_MainModuleElementStatusItem.sensorStatus (2009-06-26). */
  T_RecognitionSensorStatus   hibouStatus;               /**< @deprecated Use #T_MainModuleElementStatusItem.recognitionSensorStatus (2009-06-26). */
} T_MainModuleElementStatusItem;

typedef struct MainModuleStatus {
  T_ModuleId                     id;                       
  T_OperationalStatus            operationalStatus;
  T_MainModuleErrorCode          errorCode;             /**< Specific main module error status. @see T_MainModuleErrorCode */
  T_BillTransportStatus          billTransportStatus;   /**< Status of the bill transport. */
  T_BillTransportSection         billTransportSection;  /**< Transport section related to the billTransportStatus. Not set if billTransportStatus = BTS_OK */
  UINT32                         size;                  /**< Number of element containing in items field. */
  T_MainModuleElementStatusItem  items[NBMAXELEMENTS];  /**< Table of structures containing the status of all the elements in MainModule module. */
} T_MainModuleStatus;

typedef enum BundlerErrorCode {
  BUEC_NO_ERROR =               0,  /**< No error. */
  BUEC_CANNOT_FIND_MARK =       1,  /**< The bundler could not find the position mark during initialization cycle. */
  BUEC_CANNOT_INIT_WITH_BILLS = 2,  /**< The bundler initialization cycle could not be done because it contains bills. */
  BUEC_INIT_REQUIRED =          3,  /**< After a cycle, the position mark has not been detected. */
} T_BundlerErrorCode;

typedef enum BundlerFunctionalStatus {
  BUFS_UNKNOWN =     0,
  BUFS_POSITIONNED = 1,
} T_BundlerFunctionalStatus;

typedef enum BillStorageStatus {
  BSS_NO_ERROR =        0,  /**< No error. */
  BSS_ALMOST_EMPTY =    1,  /**< The physical cash unit bill count is lower than the low threshold value. */
  BSS_EMPTY =           2,  /**< The physical cash unit is empty. */
  BSS_ALMOST_FULL =     3,  /**< The physical cash unit bill count is higher than the high threshold value. */
  BSS_FULL =            4,  /**< The physical cash unit bill count is full. */
  BSS_NOT_TESTED =      5,  /**< @deprecated No more used. */
  BSS_WRONG_DENO =      6,  /**< @deprecated No more used. */
  BSS_BILLS_NOT_VALID = 7   /**< @deprecated No more used. */
} T_BillStorageStatus;

typedef union BundlerElementStatusItem {
  T_ElementStatus  elementStatus;
  T_SensorStatus   sensorStatus;
  T_SensorStatus   detectorStatus;  /**< @deprecated Use #T_BundlerElementStatusItem.sensorStatus (2009-06-26) */
  T_MotorStatus    motorStatus;
} T_BundlerElementStatusItem;

typedef struct BundlerStatus {
  T_ModuleId                  id;
  T_OperationalStatus         operationalStatus;
  T_BundlerErrorCode          errorCode;
  T_BundlerFunctionalStatus   functionalStatus;
  T_BillStorageStatus         billStorageStatus;
  UINT32                      size;
  T_BundlerElementStatusItem  items[NBMAXELEMENTS];
} T_BundlerStatus;

typedef enum RecyclerFunctionalStatus {
  RFS_UNKNOWN = 0,      /**< The status can not be determined. */
  RFS_POSITIONNED = 1,  /**< The recycler is positioned. */
  RFS_PARKING = 2,      /**< The recycler is moving to parked state. */
  RFS_PARKED = 3        /**< The recycler is parked. */
} T_RecyclerFunctionalStatus;

typedef enum CashTypeStatus {
  CTS_UNKNOWN,         /**< Can not determine the CashType status of the module. */
  CTS_OK,              /**< The CashType status is OK - No errors. */
  CTS_WRONG_SETTING,   /**< The CashType in the module does not correspond to its LogicalCashUnit's CashType. */
  CTS_WRONG_BILLS,     /**< The bills in the module are different than the CashType defines in the module. */
  CTS_BILLS_NOT_VALID  /**< The bills in the module correspond to its CashType, but are not recognized by the system. */
} T_CashTypeStatus;

typedef enum TapePositionFunctionalStatus {
  TPFS_UNKNOWN =            0,
  TPFS_MIN_POSITION =       1,
  TPFS_ZERO_BILL_POSITION = 2,
  TPFS_OK =                 3,
  TPFS_LAST_BILL_POSITION = 4,
  TPFS_MAX_POSITION =       5,
} T_TapePositionFunctionalStatus;

typedef enum TapePositionErrorCode {
  TPEC_NO_ERROR =             0,  /**< No error. */
  TPEC_COM_BREAKDOWN =        1,  /**< No communication with the module that owns the tape. */
  TPEC_NO_STEP_FOR_BILL_REEL= 2,  /**< Bill reel coding wheel counted no step while motor was on. */
  TPEC_NO_STEP_FOR_TAPE_REEL= 3,  /**< Tape reel coding wheel counted no step while motor was on. */
} T_TapePositionErrorCode;

typedef struct TapePositionStatus {
  T_ModuleId                      id;
  T_OperationalStatus             operationalStatus;
  T_TapePositionFunctionalStatus  functionalStatus;
  T_TapePositionErrorCode         errorCode;
} T_TapePositionStatus;

typedef union RecyclerElementStatusItem {
  T_ElementStatus       elementStatus;
  T_SensorStatus        sensorStatus;
  T_SensorStatus        detectorStatus;  /**< @deprecated Use #T_RecyclerElementStatusItem.sensorStatus (2009-06-26). */
  T_MotorStatus         motorStatus;
  T_TapePositionStatus  tapePositionStatus;
} T_RecyclerElementStatusItem;

typedef struct RecyclerStatus {
  T_ModuleId                   id;
  T_OperationalStatus          operationalStatus;
  T_ModuleErrorCode            errorCode;
  T_RecyclerFunctionalStatus   functionalStatus;
  T_BillStorageStatus          billStorageStatus;
  T_BillTransportStatus        billTransportStatus;
  T_BillTransportSection       billTransportSection;   /**< Transport section related to the billTransportStatus. Not set if billTransportStatus = BTS_OK */
  T_CashTypeStatus             cashTypeStatus;         /**< Status of the CashType compatibility between the module and its LogicalCashUnit. */
  UINT32                       size;
  T_RecyclerElementStatusItem  items[NBMAXELEMENTS];
} T_RecyclerStatus;

typedef union CashboxElementStatusItem {
  T_ElementStatus  elementStatus;
  T_SensorStatus   sensorStatus;
  T_SensorStatus   detectorStatus;  /**< @deprecated Use #T_CashboxElementStatusItem.sensorStatus (2009-06-26). */
} T_CashboxElementStatusItem;


typedef struct CashboxStatus {
  T_ModuleId                  id;
  T_OperationalStatus         operationalStatus;
  T_ModuleErrorCode           errorCode;
  T_BillStorageStatus         billStorageStatus;
  UINT32                      size;
  T_CashboxElementStatusItem  items[NBMAXELEMENTS];
} T_CashboxStatus;

typedef enum DiverterFunctionalStatus {
  DIFS_UNKNOWN =         0,  /**< The status of the diverter can not be determined. */
  DIFS_SPINE_POSITION =  1,  /**< The diverter is spine position. */
  DIFS_MODULE_POSITION = 2,  /**< The diverter is module position. */
} T_DiverterFunctionalStatus;

typedef enum DiverterErrorCode {
  DIEC_NO_ERROR =              0,  /**< No error. */
  DIEC_CANNOT_MOVE_TO_SPINE =  1,  /**< The diverter failed to move to spine position. */
  DIEC_CANNOT_MOVE_TO_MODULE = 2,  /**< The diverter failed to move to module position. */
  DIEC_CANNOT_STAY_TO_MODULE = 3,  /**< The diverter failed to stay on module position. */
  DIEC_COM_BREAKDOWN =         4,  /**< No communication with the module that owns the diverter. */
} T_DiverterErrorCode;

typedef struct DiverterStatus {
  T_ModuleId                  id;
  T_OperationalStatus         operationalStatus;
  T_DiverterFunctionalStatus  functionalStatus;
  T_DiverterErrorCode         errorCode;
} T_DiverterStatus;


typedef union SpineElementStatusItem {
  T_ElementStatus   elementStatus;
  T_CoverStatus     coverStatus;
  T_SensorStatus    sensorStatus;
  T_SensorStatus    detectorStatus;  /**< @deprecated Use #T_SpineElementStatusItem.sensorStatus (2009-06-26). */
  T_DiverterStatus  diverterStatus;
} T_SpineElementStatusItem;

typedef struct SpineStatus {
  T_ModuleId                id;
  T_OperationalStatus       operationalStatus;
  T_ModuleErrorCode         errorCode;
  T_BillTransportStatus     billTransportStatus;
  T_BillTransportSection    billTransportSection;  /**< Transport section related to the billTransportStatus. Not set if billTransportStatus = BTS_OK */
  UINT32                    size;
  T_SpineElementStatusItem  items[NBMAXELEMENTS];
} T_SpineStatus;

typedef enum LoaderFunctionalStatus {
  PFS_UNKNOWN =     0,
  PFS_POSITIONNED = 1,
  PFS_PREFEEDED =   2,
} T_LoaderFunctionalStatus;

typedef enum PressurePlateFunctionalStatus {
  PPFS_UNKNOWN =     0,  /**< The status can not be determined. */
  PPFS_POSITIONNED = 1,  /**< The plate is positioned. */
  PPFS_MOVED_DOWN =  2,  /**< The plate is in low position. */
} T_PressurePlateFunctionalStatus;

typedef enum PressurePlateErrorCode {
  PPEC_NO_ERROR,       /**< No error. */
  PPEC_COM_BREAKDOWN,  /**< No communication with the module that owns the plate. */
  PPEC_PLATE_BLOCKED,  /**< The plate is blocked. */
  PPEC_PLATE_TOO_LOW,  /**< The plate position is too low. */
  PPEC_FULL            /**< The plate is in full position. */
} T_PressurePlateErrorCode;

typedef struct PressurePlateStatus {
  T_ModuleId                       id;
  T_OperationalStatus              operationalStatus;
  T_PressurePlateFunctionalStatus  functionalStatus;
  T_PressurePlateErrorCode         errorCode;
} T_PressurePlateStatus;

typedef enum LoaderStackHeightFunctionalStatus {
  LSHFS_UNKNOWN,
  LSHFS_OK,
  LSHFS_MIN_HEIGHT,
} T_LoaderStackHeightFunctionalStatus;

typedef struct LoaderStackHeightStatus {
  T_ModuleId                           id;
  T_OperationalStatus                  operationalStatus;  /**< Always set to #OS_OPERATIONAL. */
  T_LoaderStackHeightFunctionalStatus  functionalStatus;
} T_LoaderStackHeightStatus;

typedef union LoaderElementStatusItem {
  T_ElementStatus            elementStatus;
  T_SensorStatus             sensorStatus;
  T_SensorStatus             detectorStatus;       /**< @deprecated Use #T_LoaderElementStatusItem.sensorStatus (2009-06-26). */
  T_MotorStatus              motorStatus;
  T_PressurePlateStatus      pressurePlateStatus;
  T_FlapStatus               antiFishingStatus;
  T_LoaderStackHeightStatus  stackHeightStatus;
} T_LoaderElementStatusItem;

typedef struct LoaderStatus {
  T_ModuleId                 id;
  T_OperationalStatus        operationalStatus;
  T_ModuleErrorCode          errorCode;
  T_LoaderFunctionalStatus   functionalStatus;
  T_BillStorageStatus        billStorageStatus;
  T_BillTransportStatus      billTransportStatus;
  T_BillTransportSection     billTransportSection;   /**< Transport section related to the billTransportStatus. Not set if billTransportStatus = BTS_OK */
  T_CashTypeStatus           cashTypeStatus;         /**< Status of the CashType compatibility between the module and its LogicalCashUnit. */
  UINT32                     size;
  T_LoaderElementStatusItem  items[NBMAXELEMENTS];
} T_LoaderStatus;

typedef union BarcodeReaderElementStatusItem {
  T_ElementStatus  elementStatus;
  T_SensorStatus   sensorStatus;
} T_BarcodeReaderElementStatusItem;

typedef struct BarcodeReaderStatus {
  T_ModuleId                        id;
  T_OperationalStatus               operationalStatus;
  T_ModuleErrorCode                 errorCode;
  UINT32                            size;
  T_BarcodeReaderElementStatusItem  items[NBMAXELEMENTS];
} T_BarcodeReaderStatus;

typedef union ModuleStatus {
  T_ModuleStatusBaseClass  baseClass;
  T_MainModuleStatus       mainModuleStatus;
  T_BundlerStatus          bundlerStatus;
  T_RecyclerStatus         recyclerStatus;
  T_CashboxStatus          cashboxStatus;
  T_SpineStatus            spineStatus;
  T_LoaderStatus           loaderStatus;
  T_BarcodeReaderStatus    barcodeReaderStatus;
} T_ModuleStatus;

typedef struct Modules {
  T_ModuleId  moduleId;
} T_Modules;

// BNR API Include
//#include "../BillProcessor/BNR_API/BnrCtlW32.h"

#define BILL_MMF_NAME		_T("BILL_PROCESSOR_MMF")

#define BNR_CASHTAKEN_TIME_OUT_IN_MS            (30000)
#define BNR_DEFAULT_OPERATION_TIME_OUT_IN_MS    (5000)
#define BNR_OPEN_OPERATION_TIME_OUT_IN_MS       (1000)
#define BNR_RESET_OPERATION_TIME_OUT_IN_MS      (60000)
#define BNR_CASHIN_OPERATION_TIME_OUT_IN_MS     (30000)
#define BNR_USER_TIME_OUT_IN_MS					(60000)		// 손님이 지폐를 가져가는 대기 시간

static const INT BILL_TYPE			= 4; 
static const INT MAX_BNR_ALARM_CNT	= 64;

static const INT MAX_PCU_NUM		= 7;
static const INT MAX_LCU_NUM		= 20;

static const INT MAX_CASHBOX_NUM	= 9;

// Logical Unit Number에 사용 되는 배열 인덱스
static const INT BU			= 0;
static const INT CB			= 1;
static const INT LO1		= 2;
static const INT RE3		= 3;
static const INT RE4		= 4;
static const INT RE5		= 5;
static const INT RE6		= 6;

// Physical Cash Unit Name Array
static CHAR PCUName[MAX_PCU_NUM][SIZE_OF_PHYSICAL_NAME] = {
	_T("BU"), _T("CB"), _T("LO1"), _T("RE3"), _T("RE4"), _T("RE5"), _T("RE6")
};

//----------------------------------------------------------------------//
// 구조체 "bill_count" 배열 참조에 사용되는 인덱스 정의

// recycler 보유량 배열 인덱스
static const INT H_RE3		= 0;
static const INT H_RE4		= 1;
static const INT H_RE5		= 2;
static const INT H_RE6		= 3;

// 5000, 10000 권 recycler 인덱스
static const INT B_5000		= 4;
static const INT B_10000	= 3;

// 보유량에 사용되는 cash box 배열 Index
// 구조체 "bill_count" 배열 참조
static const INT CB_1000	= 0;
static const INT CB_5000	= 1;
static const INT CB_10000	= 2;
static const INT CB_50000	= 3;
static const INT CB_UNKNOWN = 4;

static const INT BANKNOTE_1000   = 0;
static const INT BANKNOTE_5000   = 1;
static const INT BANKNOTE_10000  = 2;
static const INT BANKNOTE_50000  = 3;

// Modules ID에 사용되는 배열 Index
static const INT32	MAIN_MODULE  = 0;
static const INT32	BUNDLER		 = 1;
static const INT32	SPINE		 = 2;
static const INT32	LOADER		 = 3;
static const INT32	RECYCLER3	 = 4;
static const INT32	RECYCLER4	 = 5;
static const INT32	RECYCLER5	 = 6;
static const INT32	RECYCLER6	 = 7;
static const INT32	CASH_BOX	 = 8;

// 지폐장치 제조사 (device_type)
static const INT MEI_BNR	 = 1;	// 지폐환류기
static const INT TOYOCOM_BNK = 2;	// 지폐처리장치 (도요콤)

//----------------------------------------------------------------------//
// Structure Declare
//----------------------------------------------------------------------//
#ifndef WIN32
	#pragma pack(push,1)
#else
	#include <pshpack1.h>
#endif

// cash unit 정보 구조체
typedef struct  cashunit_information
{	
	BOOL		lock;
	UINT32		num;
	INT			value;			
	UINT32		b_count;
}T_CASH_UNIT_INFO;

typedef struct  cashunit_information T_BILL_ESCROW;
typedef struct  cashunit_information T_BILL_CASSETTE;

// lock 
typedef struct  
{
	CHAR		module_name[5];
	BOOL		lock;
}T_CU_LOCK;


typedef struct  
{
	CHAR		module_name[5];
	INT         cash_cnt;
}T_CASHCNT_CHANGE;

// 투입 및 회수 정보 구조체
typedef struct bill_amount
{
	UINT32	count[7];       // 2:LO1, 3:RE3, 4:RE4, 5:RE5, 6:RE6 (지폐환류장치)
							// 0: 1000원, 1: 5000원, 2: 10000원, 3: 50000원 (지폐처리장치)
	UINT32  cashbox_cnt[5]; 
	DWORD	amount;
}T_BILL_IN_AMOUNT, T_CASHBOX_COLLECT, T_BILL_SUPPLY, T_BILL_COLLECT;


// 거스름 정보 구조체
typedef struct bill_change
{	
	UINT32 count[5];	// 0:LO1, 1:RE3, 2:RE4, 3:RE5, 4:RE6
	INT  amount;
}T_BILL_OUT_AMOUNT;


// module status 
typedef struct  
{	
}T_MODULE_STATUS;


// error 
typedef struct  
{
	TCHAR alarm_key[5];
	TCHAR modulecode;
	TCHAR alarmcode[4];	
}T_BILL_ALARM;


typedef struct _billstatus
{
	BOOL				connect;
	T_MODULE_STATUS		modules;
}T_BILL_STATUS;


// 재고 정보
typedef struct bill_count
{
	DWORD CountOfBU[4];	// Bundler 지폐 수량
	DWORD CountOfLO;	// Loader   지폐 수량
	DWORD CountOfRE[4];	// Recycler 지폐 수량
	DWORD CountOfCB[5];	// cash box 지폐 수량
	DWORD TotalAmount;
}T_HOLDING_AMOUNT, T_BILL_HOLDING, T_TEMP_INSERT;


typedef struct bill_error_count
{
	DWORD jam_amount;
	DWORD unr_amount;
}T_ERROR_AMOUNT;


// share data 
typedef struct  
{
	TCHAR                   _ver[11];
	// operation
	INT						main_cmd;								// main->device operation code
	INT                     device_cmd;								// device->main command

	// status
	INT						n_result;								// operation result								
	INT						alarm_cnt;								// module error count
	INT                     transaction_mode;
	BOOL					device_open;
	BOOL                    enabel_cashbox_clr;						// ACM용 지폐함 탈장착 시 초기화
	bool                    bLoaderStatusChange;
	bool                    bCashboxStatusChange;

	T_BILL_ALARM			bill_alarm[MAX_BNR_ALARM_CNT];			// module error code array

	T_BILL_STATUS			status;
	BYTE					bnr_status;

	// configration	
	T_CASHCNT_CHANGE        cash_change;                            // 수량 변경
	T_CU_LOCK				module_lock;							// LO, RE 잠김 상태
	INT						empty[MAX_PCU_NUM];						// 모듈 별 회수 플래그
	INT						device_type;							// 지폐환류장치:0, 지폐처리장치:1

	// audit														
	DWORD					dw_unrAmt;								// 미방출 금액
	DWORD					dw_overAmt;								// 과방출 금액
	DWORD					dw_jamAmt;								// JAM 발생 플래그
	T_BILL_IN_AMOUNT		amt_input;								// 투입 금액/수량 정보
	T_BILL_OUT_AMOUNT		amt_output;								// 방출 금액/수량 정보	
	T_HOLDING_AMOUNT		holding_amt;							// 보유량 정보
	T_BILL_SUPPLY           bill_supply;                            // 보급 정보	
	T_BILL_COLLECT          bill_collect;                           // 지급 정보
}T_BILL_DATA;

// restore packing
#ifndef WIN32
	#pragma pack(pop)
#else
	#include <poppack.h>
#endif

//----------------------------------------------------------------------//
// CBillProcessorData class declare
//----------------------------------------------------------------------//
class CBillProcessorData 
{
public:
	CBillProcessorData(IN LPCTSTR lpMapName, IN const SIZE_T &nBytes, IN BOOL bOpen=FALSE)
	{
	};

	virtual ~CBillProcessorData(){};

	T_BILL_DATA *GetMMData() {
	}

private:
	T_BILL_DATA	*m_pData;
};

