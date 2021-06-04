%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Main (1st script you need to execute)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Metadata
% Written by    : Nathanaël Esnault
% Verified by   : N/A
% Creation date : 2021-04-09
% Version       : 0.1 (finished on ...)
% Modifications :
% Known bugs    :

%% Functions associated with this code :

%% Possible Improvements

% TODO

%% Cleaning + testID
close all;
clc
clearvars;
% Generate a unique test ID for this Matlab run
formatOut = 'yyyy-mm-dd--HH-MM-SS-FFF';
RunID = datestr(now,formatOut);
clearvars formatOut;
format compact  % Suppress excess blank lines to show more output on a single screen.
format longG    % Disable those "e+09" when dealing with indeces

%% Work Matlab has to do
Do.SaveFigures              = 0; % Not programmed
Do.GenerateStatistics       = 1; % Not programmed
Do.GenerateReport           = 0; % Not programmed
Do.SavePNGPlots             = 0; % Not programmed
Do.SaveData                 = 0;
Do.SaveDataAsTxT            = 0;
Do.Plots                    = 1;
Do.DockWindows              = 1;
Do.ApplyLowPassFilter       = 1;
Do.SaveCalibratedAsMat      = 0;
Do.ResampleDataToMs         = 1; % <-----
Do.GenerateSimulinkData     = 0;
Do.ZuptGyro                 = 0; %
Do.RemoveGravityFromAccelerations = 1;
Do.SLERPsmoothing           = 0; % <-----
% Also chose moving average of the gyro

%% Parameters #1
% GetParameters; % Not programmed

if Do.DockWindows
    set(0,'DefaultFigureWindowStyle','docked');
end

indx_X = 1;
indx_Y = 2;
indx_Z = 3;


IMU_Type                    = 'LSM6DS33'; % NEVER edit that
inputFolder                 = '00_PUT_SD_CARD_DATA_HERE'; % AVOID editing that
ft                          = 'yyyy_mm_dd__HH_MM_SS'; %for the timestamp IN the TXT file
ft_file                     = 'yyyy_mm_dd__HH_MM_SS'; %for the name OF the TXT file

%% Parameters #2 (EDIT me BEFORE running the script!!!!)

IMU_number                  = 3; % This tells Matlab which pontoon the IMU was located at, can be 1, 2, 3 or 4
AquisitionName              = ' - Short Office Test IMU#3  FW 240MHz Boost';

displayTimeFormat           = 's'; % can be 's'
lowPassCutOffFrequency      = 4; % in [Hz]
AutochoseSavePath           = 1; %  for saving the files % Not programmed

SampleRate_LSM9DS33         = 10; % Hz
LogModeLength               = 18000; % max quantity of lines/ messages per file, you can change that for memory optimisation
IMUFilterType               = 'Kalman'; % can be either 'Kalman' or 'Complementary'

% IMU_orientationName = 'WND'; % Not programmed (Checked, respects the right-hand rule)
% IMU_orientationMatrix = [...
%     [0 +1 0];...
%     [-1 0 0];...
%     [0 0 +1] ...
%     ]; % Not programmed, TODO: should be calculated automatically



%% Load data
% Make sure the data are converted to: m/s2, rad/s, (microTesla)
switch IMU_Type
    case 'LSM6DS33'
        
        DealWithMultipleFiles; % import data + time resampling
        
        % deal with time
        SampleRate      = SampleRate_LSM9DS33;
        globalTime      = seconds(time_LSM9DS33 - time_LSM9DS33(1)); % this will also be used to plot
        
        % Change the orientation to NED (from most probably WNU)
        accelReadings   =    [+allData.AccY -allData.AccX +allData.AccZ];
        rawAcc = accelReadings;
        gyroReadings    =    [+allData.GyrY -allData.GyrX +allData.GyrZ];
        
        
        % We actually don't need that variable anymore
        %         clearvars allData
        
    otherwise
        disp('Wrong IMU type');
        occuredError = 1;
end

%---------------------------------------------------------------------------------
figure
% subplot(2,1,1)
% plot(globalTime ,'-o')
% grid on;
% axis tight
% title('Timestamp check (From each data line)');
% ylabel('Time [s]');
% xlabel('Timestamp index [N/A]');

% subplot(2,1,2)
plot(diff(globalTime) ,'-o')
grid on;
title('Variation of timestamp check (From each data line)');
ylabel('Time variation [s]');
xlabel('Timestamp index [N/A]');


%% Filter (remove DC + low pass) everyone

% Check the variance of the signals before applying the filtering
Variance.Acc.raw = [...
    std(accelReadings(:,indx_X))...
    std(accelReadings(:,indx_Y))...
    std(accelReadings(:,indx_Z))...
    ];
Variance.Gyr.raw = [...
    std(gyroReadings(:,indx_X))...
    std(gyroReadings(:,indx_Y))...
    std(gyroReadings(:,indx_Z))...
    ];


if Do.ApplyLowPassFilter
    
    tic
    
    % apply a moving average on the gyros
    gyroReadings = [...
        gyroReadings(:,indx_X) - movmean(gyroReadings(:,indx_X),1e5),...
        gyroReadings(:,indx_Y) - movmean(gyroReadings(:,indx_Y),1e5),...
        gyroReadings(:,indx_Z) - movmean(gyroReadings(:,indx_Z),1e5) ...
        ];
    
    %     figure
    %     plot(gyroReadings(:,indx_Y))
    %     hold on
    %     plot(detrend(gyroReadings(:,indx_Y)))
    %     hold on
    %     plot(gyroReadings(:,indx_Y) - movmean(gyroReadings(:,indx_Y),1e5))
    %     grid on
    %     legend('Normal', 'detrend', '1e5');
    
    %             figure
    %             plot(gyroReadings(:,indx_X))
    %             hold on
    %             plot(movmean(gyroReadings(:,indx_X),10))
    %             hold on
    %             plot(movmean(gyroReadings(:,indx_X),1e3))
    %             hold on
    %             plot(movmean(gyroReadings(:,indx_X),1e4))
    %             hold on
    %             plot(movmean(gyroReadings(:,indx_X),1e5))
    %             grid on
    legend('Normal', '10', '1e3', '1e4', '1e5');
    
    % Apply low-pass filter (it is improbable wave will induce motions faster than 1Hz on pontoons)
    gyroReadings(:,indx_X) = lowpass(gyroReadings(:,indx_X),lowPassCutOffFrequency,SampleRate);
    gyroReadings(:,indx_Y) = lowpass(gyroReadings(:,indx_Y),lowPassCutOffFrequency,SampleRate);
    gyroReadings(:,indx_Z) = lowpass(gyroReadings(:,indx_Z),lowPassCutOffFrequency,SampleRate);
    
    accelReadings(:,indx_X) = lowpass(accelReadings(:,indx_X),lowPassCutOffFrequency,SampleRate);
    accelReadings(:,indx_Y) = lowpass(accelReadings(:,indx_Y),lowPassCutOffFrequency,SampleRate);
    accelReadings(:,indx_Z) = lowpass(accelReadings(:,indx_Z),lowPassCutOffFrequency,SampleRate);
    
    % Check the variance of the signals to see if the filtering improved the
    %noise
    Variance.Acc.filtered = [...
        std(accelReadings(:,indx_X))...
        std(accelReadings(:,indx_Y))...
        std(accelReadings(:,indx_Z))...
        ];
    Variance.Gyr.filtered = [...
        std(gyroReadings(:,indx_X))...
        std(gyroReadings(:,indx_Y))...
        std(gyroReadings(:,indx_Z))...
        ];
    
    % Calculate variance improvements
    Variance.Acc.ImprovementPercent = ...
        (Variance.Acc.raw - Variance.Acc.filtered) ./ Variance.Acc.raw * 100;
    Variance.Gyr.ImprovementPercent = ...
        (Variance.Gyr.raw - Variance.Gyr.filtered) ./ Variance.Gyr.raw * 100;
    
    toc % for timing purposes
end


%% Minimise gyro drift
if Do.ZuptGyro
    
    disp('Zupting the gyros and accel...');
    
    
end

%% Pose estimation (calculate the Euler angles from the IMU calibrated data)
% Use a matlab system object called imufilter filter to generate attitudes from
% calibrated IMU data (3 acc 3 gyr)

% Accelerometer readings in the sensor body coordinate system should be in
% [m/s2]
% Gyroscope readings in the sensor body coordinate system should be in
% [rad/s]

% Specify RF as 'NED' (North-East-Down) or 'ENU' (East-North-Up). The
% default value is 'NED'.

disp('Fusing...');
tic
switch IMUFilterType
    case 'Complementary'
        fuse  					= imufilter('SampleRate',SampleRate); % use the same structure for every IMU
    case 'Kalman'
        fuse  					= complementaryFilter('SampleRate', SampleRate, 'HasMagnetometer', false);
    otherwise
        disp('Unknown filter type');
        occuredError = 1;
end

orientation 			= fuse(accelReadings,gyroReadings); % outputs a quaternion
orientationEulerAngles 	= eulerd(orientation,'XYZ','frame'); % order in: accel, gyro | order out: roll, pitch yaw

disp(['Fusing done in ' num2str(toc) ' s']);
% clearvars fuse

figure
plot(unwrap(orientationEulerAngles(:,1)))
hold on
plot(unwrap(orientationEulerAngles(:,1)) - movmean(unwrap(orientationEulerAngles(:,1)),1e5))
grid on
figure
plot(unwrap(orientationEulerAngles(:,2)))
hold on
plot(unwrap(orientationEulerAngles(:,2)) - movmean(unwrap(orientationEulerAngles(:,2)),1e5))
grid on
figure
plot(unwrap(orientationEulerAngles(:,3)))
hold on
plot(unwrap(orientationEulerAngles(:,3)) - movmean(unwrap(orientationEulerAngles(:,3)),1e5))
grid on

%% SLERP smoothing (recursive)

if Do.SLERPsmoothing
    % Parameters
    hrange          = 0.4;
    hbias           = 0.4;
    lowLim          = max(min(hbias - (hrange./2), 1), 0);
    highLim         = max(min(hbias + (hrange./2), 1), 0);
    hrangeLimited   = highLim - lowLim;
    
    % Initialize the filter and preallocate outputs
    orientation_slerped     = zeros(size(orientation), 'like', orientation); % preallocate filter output
    orientation_slerped(1)  = orientation(1); % initial filter state
    
    %waitbar
    f = waitbar2(0,'Filtering quaternion by SLERP');
    
    % Filter the noisy trajectory, sample-by-sample (recursive)
    for cnt_iter = 2 : numel(orientation)
        % Update waitbar and message
        waitbar2(cnt_iter/numel(orientation),f,...
            [num2str(cnt_iter/numel(orientation)*100) '%' ]);
        
        % calculate distance
        dist_quat = dist(orientation_slerped(cnt_iter-1), orientation(cnt_iter));
        
        % Renormalize dist output to the range [low, high]
        hlpf = (dist_quat./pi) .* hrangeLimited + lowLim;
        orientation_slerped(cnt_iter) = slerp(orientation_slerped(cnt_iter-1),orientation(cnt_iter),hlpf);
    end
    
    % Remove waitbar window
    delete(f);
    
    % orientationEulerAngles_slerped 	= eulerd(orientation_slerped,'ZYX','frame'); % order in: accel, gyro | order out: roll, pitch yaw
    
    % debug plot: see the effect of the SLERP
    % figure()
    % plot(globalTime_plot,unwrap(orientationEulerAngles(:,3)))
    % hold on
    % plot(globalTime_plot,unwrap(orientationEulerAngles_slerped(:,3)))
    % xlabel(plotAxisLabelTime)
    % ylabel('Angle [°]')
    % title('Orientation over Time')
    % grid on
    
end

%% Position calculation (calculate the surge sway and heave from the IMU calibrated data)

% This one is going to be calculated by double integration, this is going
% to diverge quickly and have a lot of noise. Also because the IMU chosen
% is not the best, even for the "personal" application range

% step #0 preallocation
% positionPontoon    = nan(length(globalTime), 3);

% step #1 remove gravity from data
% Caution: 'quatrotate' requires Aerospace Toolbox.
% clearvars orientation
% gravityVector      = [0 0 9.81]; % the acceleration to remove (was supposed to be expressed in NED to be consistant)
% % % % % gravityToRemove = repmat(gravityVector,10,1); %tempoary fix
% gravityToRemove    = quatrotate(compact(orientation(:)), gravityVector);
% pureMouvementAccel = accelReadings(:,:)  -  gravityToRemove(:,:);

pureMouvementAccel = [...
    accelReadings(:,indx_X) - movmean(accelReadings(:,indx_X),1e5),...
    accelReadings(:,indx_Y) - movmean(accelReadings(:,indx_Y),1e5),...
    accelReadings(:,indx_Z) - movmean(accelReadings(:,indx_Z),1e5) ...
    ];

% Remove gravity from raw acceleration
PontoonData_RawAcc_X = rawAcc(:,indx_X) - movmean(rawAcc(:,indx_X),1e5);
PontoonData_RawAcc_Y = rawAcc(:,indx_Y) - movmean(rawAcc(:,indx_Y),1e5);
PontoonData_RawAcc_Z = rawAcc(:,indx_Z) - movmean(rawAcc(:,indx_Z),1e5);

% step #2 remove the initial offsets
% pureMouvementAccel(:,indx_X) = pureMouvementAccel(:,indx_X) - pureMouvementAccel(1,indx_X);
% pureMouvementAccel(:,indx_Y) = pureMouvementAccel(:,indx_Y) - pureMouvementAccel(1,indx_Y);
% pureMouvementAccel(:,indx_Z) = pureMouvementAccel(:,indx_Z) - pureMouvementAccel(1,indx_Z);

% step #3 double integration
deltaT = [0; diff(globalTime)];
positionPontoon(:,indx_X) = cumsum(cumsum(pureMouvementAccel(:,indx_X)) .* deltaT) .* deltaT;
positionPontoon(:,indx_Y) = cumsum(cumsum(pureMouvementAccel(:,indx_Y)) .* deltaT) .* deltaT;
positionPontoon(:,indx_Z) = cumsum(cumsum(pureMouvementAccel(:,indx_Z)) .* deltaT) .* deltaT;
% positionPontoon = [...
%     cumsum(cumsum(pureMouvementAccelZupt(:,indx_X)) .* deltaT) .* deltaT,...
%     cumsum(cumsum(pureMouvementAccelZupt(:,indx_Y)) .* deltaT) .* deltaT,...
%     cumsum(cumsum(pureMouvementAccelZupt(:,indx_Z)) .* deltaT) .* deltaT ...
%     ];


%% Plot
C_Plots; % just call the script

%% Save data as specified by Eddie in the email

% save as a .mat file + timeseries, 1 per unit
% time, 3 processed acc, 3 Euler orientations, 3 positions

% orientation of the IMU enclosure is tacken as a treasure chest

% % % if Do.RemoveGravityFromAccelerations
% % %     PontoonDataTimeSeries = timeseries(...
% % %         [...
% % %         pureMouvementAccel(:,indx_X),... % hinges of the enclosure
% % %         pureMouvementAccel(:,indx_Y),... % right of the enclosure of the enclosure, cable gland
% % %         pureMouvementAccel(:,indx_Z),... % bottom of the enclosure of the enclosure
% % %         positionPontoon(:,indx_X),...
% % %         positionPontoon(:,indx_Y),...
% % %         positionPontoon(:,indx_Z),...
% % %         unwrap(orientationEulerAngles(:,3)) - movmean(unwrap(orientationEulerAngles(:,3)),1e5),... % roll
% % %         unwrap(orientationEulerAngles(:,2)) - movmean(unwrap(orientationEulerAngles(:,2)),1e5),... % pitch
% % %         unwrap(orientationEulerAngles(:,1)) - movmean(unwrap(orientationEulerAngles(:,1)),1e5)...  % yaw
% % %         ]...
% % %         , globalTime,...
% % %         'Name', 'AccelPositionOrientation');
% % % else
% % %     PontoonDataTimeSeries = timeseries(...
% % %         [...
% % %         accelReadings(:,indx_X),... % hinges of the enclosure
% % %         accelReadings(:,indx_Y),... % right of the enclosure of the enclosure, cable gland
% % %         accelReadings(:,indx_Z),... % bottom of the enclosure of the enclosure
% % %         positionPontoon(:,indx_X),...
% % %         positionPontoon(:,indx_Y),...
% % %         positionPontoon(:,indx_Z),...
% % %         orientationEulerAngles(:,indx_Z),... % roll
% % %         orientationEulerAngles(:,indx_Y),... % pitch
% % %         orientationEulerAngles(:,indx_X)...  % yaw
% % %         ]...
% % %         , globalTime,...
% % %         'Name', 'AccelPositionOrientation');
% % % end

PontoonData_DateTime_Str =        (time_LSM9DS33);
PontoonData_DateTime_Num = datenum(time_LSM9DS33);

% Remove gravity from raw acceleration
% PontoonData_FilteredAcc_X = accelReadings(:,indx_X);
% PontoonData_FilteredAcc_Y = accelReadings(:,indx_Y);
% PontoonData_FilteredAcc_Z = accelReadings(:,indx_Z);
PontoonData_FilteredAcc_X = accelReadings(:,indx_X) - movmean(accelReadings(:,indx_X),1e5);
PontoonData_FilteredAcc_Y = accelReadings(:,indx_Y) - movmean(accelReadings(:,indx_Y),1e5);
PontoonData_FilteredAcc_Z = accelReadings(:,indx_Z) - movmean(accelReadings(:,indx_Z),1e5);

PontoonData_Overall_g = sqrt(PontoonData_FilteredAcc_X.^2 + PontoonData_FilteredAcc_Y.^2 + PontoonData_FilteredAcc_Z.^2)/9.81;

% PontoonData_RawAcc_X = accelReadings(:,indx_X);
% PontoonData_RawAcc_Y = accelReadings(:,indx_Y);
% PontoonData_RawAcc_Z = accelReadings(:,indx_Z);

PontoonData_Position_X = positionPontoon(:,indx_X);
PontoonData_Position_Y = positionPontoon(:,indx_Y);
PontoonData_Position_Z = positionPontoon(:,indx_Z);

PontoonData_Roll    = unwrap(orientationEulerAngles(:,3)) - movmean(unwrap(orientationEulerAngles(:,3)),1e5);
PontoonData_Pitch   = unwrap(orientationEulerAngles(:,2)) - movmean(unwrap(orientationEulerAngles(:,2)),1e5);
PontoonData_Yaw     = unwrap(orientationEulerAngles(:,1)) - movmean(unwrap(orientationEulerAngles(:,1)),1e5);


% SaveSensorDataFileName = [datestr(listFile(1),ft_file) ' - Pier4 Deployment1 1_20DegOrientation.mat'];
SaveSensorDataFileName = [datestr(listFile(1),ft_file) AquisitionName '.mat'];
save(SaveSensorDataFileName,...
    'PontoonData_DateTime_Str',...
    'PontoonData_DateTime_Num',...
    'PontoonData_FilteredAcc_X',...
    'PontoonData_FilteredAcc_Y',...
    'PontoonData_FilteredAcc_Z',...
    'PontoonData_RawAcc_X',...
    'PontoonData_RawAcc_Y',...
    'PontoonData_RawAcc_Z',...
    'PontoonData_Position_X',...
    'PontoonData_Position_Y',...
    'PontoonData_Position_Z',...
    'PontoonData_Roll',...
    'PontoonData_Pitch',...
    'PontoonData_Yaw' ...
    );


%% Simulink model (animation)

if Do.GenerateSimulinkData
    % optimise and put in 1 vector -> careful, Simulink wants 3 different
    % vectors
    % it is necessary to unwrap the angles otherwise there will be a jump in
    % the simulink block video
    roll    = timeseries(unwrap(orientationEulerAngles(:,indx_Z)*pi/180)*180/pi - movmean(unwrap(orientationEulerAngles(:,indx_Z)*pi/180)*180/pi, 8e2),globalTime);
    pitch   = timeseries(unwrap(orientationEulerAngles(:,indx_Y)*pi/180)*180/pi - movmean(unwrap(orientationEulerAngles(:,indx_Y)*pi/180)*180/pi, 8e2),globalTime);
    yaw     = timeseries(unwrap(orientationEulerAngles(:,indx_X)*pi/180)*180/pi - movmean(unwrap(orientationEulerAngles(:,indx_X)*pi/180)*180/pi, 8e2),globalTime);
    %     yaw     = timeseries(zeros(length(pitch.Data),1),globalTime);
    figure()
    plot(unwrap(orientationEulerAngles(:,indx_X)*pi/180)*180/pi)
    %     hold on
    plot(unwrap(orientationEulerAngles(:,indx_X)*pi/180)*180/pi - movmean(unwrap(orientationEulerAngles(:,indx_X)*pi/180)*180/pi, 2e4))
    grid on
    
    %Reset orientation so the pitch and roll (and yaw) in the water are 0, even if in
    %reality , the IMU has an offset
    pitch.Data  = pitch.Data    -   pitch.Data(1);
    roll.Data   = roll.Data     -   roll.Data(1);
    yaw.Data    = yaw.Data      -   yaw.Data(1);
end



% END OF SCRIPT