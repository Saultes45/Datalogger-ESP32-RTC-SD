%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Recursive import in the designated folder (NOT Main)
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


%% Initialisation
occured_error   = 0; %DO NOT EDIT
nbrbadfiles     = 0; %DO NOT EDIT


%%
% Look for the folder named Logs
if (isfolder(inputFolder)) % use "isdir" if this line gives an error
    % look for all the files name: e.g. 2021_04_15__15_34_12-00001.txt
    
    
    %% Import
    
    % List txt files in designated folder
    MyDirInfo = dir([inputFolder '\*.txt']); %keep the star (wildcard)
    
    % Prealloc (need prev. line)
    listFile = NaN(length(MyDirInfo),1);
    indxFile = NaN(length(MyDirInfo),1);
    
    % Verify the date in the file names    
    for cnt_files = 1 : length(MyDirInfo)
        try
            [part1,part2]   = strtok(MyDirInfo(cnt_files).name, '-');
            part2          = strtok(part2(2:end), '.');
            listFile(cnt_files) = datenum(part1,ft_file);
            indxFile(cnt_files) = str2double(part2);
            clearvars token remain part1 part2
        catch
            nbrbadfiles = nbrbadfiles + 1;
        end
    end
    
    % delete remaining NaN
    listFile(isnan(listFile)) = [];
    
    %% Preallocate
    
    nbr_GoodFiles             = length(listFile);
    allData.timeStamp         = NaT(LogModeLength * nbr_GoodFiles, 1);
    allData.AccX              = NaN(LogModeLength * nbr_GoodFiles, 1);
    allData.AccY              = NaN(LogModeLength * nbr_GoodFiles, 1);
    allData.AccZ              = NaN(LogModeLength * nbr_GoodFiles, 1);
    allData.GyrX              = NaN(LogModeLength * nbr_GoodFiles, 1);
    allData.GyrY              = NaN(LogModeLength * nbr_GoodFiles, 1);
    allData.GyrZ              = NaN(LogModeLength * nbr_GoodFiles, 1);
    
    
    %% Open each file and check the timestamps (fisrt part of the line)
    idxstart = 1; % init, do NOT edit
    
    % Options common to all files
%     opts = delimitedTextImportOptions("NumVariables", 8);
opts = delimitedTextImportOptions("NumVariables", 7);
    opts.DataLines          = [1, Inf];
    opts.Delimiter          = ",";
%     opts.VariableNames      = ["Timestamp", "Tmp", "AccX", "AccY", "AccZ", "GyrX", "GyrY", "GyrZ"];
%     opts.VariableTypes      = ["string", "double", "double", "double", "double", "double", "double", "double"];
    opts.VariableNames      = ["Timestamp", "AccX", "AccY", "AccZ", "GyrX", "GyrY", "GyrZ"];
    opts.VariableTypes      = ["string", "double", "double", "double", "double", "double", "double"];
    opts.ExtraColumnsRule   = "ignore";
    opts.EmptyLineRule      = "read";
    opts = setvaropts(opts, "Timestamp", "WhitespaceRule", "preserve");
    opts = setvaropts(opts, "Timestamp", "EmptyFieldRule", "auto");
    
    % Prepare waitbar
%     f = waitbar(0,'1','Name','Importing data file by file');
    f = waitbar2(0,'Importing data file by file');
    
    for cnt_files = 1 : nbr_GoodFiles
        
        % Update waitbar and message
        waitbar2(cnt_files/nbr_GoodFiles,f,...
            [num2str(cnt_files/nbr_GoodFiles*100) '%' ]);
        
        filename = [cd '\' inputFolder '\' datestr(listFile(cnt_files),ft_file) '-' num2str(indxFile(cnt_files), '%.5d') '.txt'];
        
        try
            try
                % Import the data
                tbl = readtable(filename, opts);
            catch
                % if the fisrt line has a problem (incomplete because of power cut/log)
                opts.DataLines = [2, Inf];
                % Import the data with new options
                tbl = readtable(filename, opts);
            end
            
            idxstop = idxstart + height(tbl) - 1;
            allData.AccX(idxstart:idxstop)    = tbl.AccX;
            allData.AccY(idxstart:idxstop)    = tbl.AccY;
            allData.AccZ(idxstart:idxstop)    = tbl.AccZ;
            allData.GyrX(idxstart:idxstop)    = tbl.GyrX;
            allData.GyrY(idxstart:idxstop)    = tbl.GyrY;
            allData.GyrZ(idxstart:idxstop)    = tbl.GyrZ;
            
            
            % Check that all the lines have the SOM (ie '$')
            % rowfun doesn't work for that case
            is_SOM = nan(height(tbl), 1);
            for cnt_row = 1 : height(tbl)
                temp = tbl.Timestamp(cnt_row);
                is_SOM(cnt_row) = strfind(temp, '$');
            end
            tbl.Timestamp = strrep(tbl.Timestamp,'$','');
            allData.timeStamp(idxstart:idxstop,1) = datetime(datenum(tbl.Timestamp, ft),'ConvertFrom','datenum');
            clearvars tbl
            
            idxstart = 1 + idxstop; % increment the counter
            
        catch % second attempt to read the table
            % Do or do not, second try there is not
            disp(['Problem at iteration ' num2str(cnt_files) ' for file ' filename]);
        end
    end
    
    delete(f); % for the waitbar
    clearvars opts
    
    % Remove any NaN or empty
    % 1) search ...
    idxligndlt = find((...
        isnan(allData.AccX) |...
        isnan(allData.AccY) |...
        isnan(allData.AccZ) |...
        isnan(allData.GyrX) |...
        isnan(allData.GyrY) |...
        isnan(allData.GyrZ) |...
        isnat(allData.timeStamp)...
        ));
    % 2) ... and destroy
    allData.timeStamp   (idxligndlt,:) = [];
    allData.AccX        (idxligndlt,:) = [];
    allData.AccY        (idxligndlt,:) = [];
    allData.AccZ        (idxligndlt,:) = [];
    allData.GyrX        (idxligndlt,:) = [];
    allData.GyrY        (idxligndlt,:) = [];
    allData.GyrZ        (idxligndlt,:) = [];
    
%     %remove the start of the file
%     allData.timeStamp   (1:140,:) = [];
%     allData.AccX        (1:140,:) = [];
%     allData.AccY        (1:140,:) = [];
%     allData.AccZ        (1:140,:) = [];
%     allData.GyrX        (1:140,:) = [];
%     allData.GyrY        (1:140,:) = [];
%     allData.GyrZ        (1:140,:) = [];
    
    %% Resample s to ms
    if Do.ResampleDataToMs
        
        allData.timeStamp.Format  = 'yyyy/MM/dd hh:mm:ss.SSS';
        % Find all the datetime that are similar
        
%         f = waitbar2(0,'1','Name','Resampling the time to ms');
        f = waitbar2(0,'Resampling the time to ms');
        
        indx_timecheck_start = 1; % init, do NOT edit
        list_similar = nan(length(allData.timeStamp),1);
        while indx_timecheck_start < length(allData.timeStamp)
            try
                % Update waitbar and message
                waitbar2(indx_timecheck_start/length(allData.timeStamp),f,...
                    [num2str(indx_timecheck_start/length(allData.timeStamp)*100) '%' ]);
                
                % indx_start_seach = max([1 indx_timecheck_start]);
                indx_stop_search = min([indx_timecheck_start + SampleRate_LSM9DS33  length(allData.timeStamp)]); % the final hidden "+1" is for security
                indx_same = find(allData.timeStamp(indx_timecheck_start:indx_stop_search) == allData.timeStamp(indx_timecheck_start));
                
                indx_same = indx_same + indx_timecheck_start - 1; % adjust the offset due to the limitation of the search window
                
                if length(indx_same) > 1
                    ms_to_add = milliseconds(linspace(0, 1000, length(indx_same)+1))';
                    ms_to_add = ms_to_add(1:end-1);
                    allData.timeStamp(indx_same) = allData.timeStamp(indx_same) + ms_to_add;
                    list_similar(indx_same(1)) = length(indx_same);
                end
                indx_timecheck_start = indx_timecheck_start + length(indx_same);
            catch
                indx_timecheck_start = indx_timecheck_start + 1; % in case of an error, just increment by 1 and retry
            end
        end
        delete(f); % for the waitbar
    end
    
    %% Final parameters
    
    % We need to do that here because there we needed to remove bad
    % variables asap
    Measured_Average_SampleRate_LSM9DS33 = seconds(mean(diff(allData.timeStamp)));
    Measured_Average_Frequency_LSM9DS33 = 1 / Measured_Average_SampleRate_LSM9DS33;
    
    % Make sure the time is set to 0 for the 1st data (reference)
    time_LSM9DS33 = allData.timeStamp;
    
    
    %% Display statistics
    disp(['Initial number of good files detected: ' num2str(nbr_GoodFiles)]);
    disp(['Final number of good files detected:   ' num2str(length(listFile))]);
    %     disp(['First index of good files detected:    ' num2str((listFile(1)))]);
    %     disp(['Last index of good files detected:     ' num2str((listFile(end)))]);
    %     disp(['Theoritical number of files (from files names):     ' num2str(listFile(end) - listFile(1) + 1) ]);
    disp(['Final number of good lines detected:   ' num2str(length(allData.timeStamp))]);
    
    %% Figures
    
    %---------------------------------------------------------------------------------
    figure
    subplot(2,1,1)
    plot(listFile,'-o')
    grid on;
    title('FileName check (timestamp of the filename as datenum)');
    ylabel('File Name ID [N/A]');
    xlabel('File number [N/A]');
    
    subplot(2,1,2)
    plot(diff(listFile) ,'-o')
    grid on;
    title('diff(FileName) check (timestamp of the filename as datenum)');
    ylabel('Diff(File Name ID) [N/A]');
    xlabel('File number [N/A]');
    %---------------------------------------------------------------------------------
    figure
    subplot(2,1,1)
    plot(indxFile,'-o')
    grid on;
    title('Index associated to the file name (Should be continuously increasing)');
    ylabel('File Index (created by the datalogger) [N/A]');
    xlabel('File number (counted by Matlab) [N/A]');
    
    subplot(2,1,2)
    plot(diff(indxFile) ,'-o')
    grid on;
    title('diff(FileName) Check');
    ylabel('Variation of the file Index (created by the datalogger) [N/A]');
    xlabel('File number (counted by Matlab) [N/A]');
    %---------------------------------------------------------------------------------
    figure
    subplot(2,1,1)
    plot(allData.timeStamp ,'-o')
    grid on;
    axis tight
    title('Timestamp check (From each data line)');
    ylabel('Time');
    xlabel('Timestamp index [N/A]');
    
    subplot(2,1,2)
    plot(diff(allData.timeStamp) ,'-o')
    grid on;
    title('Variation of timestamp check (From each data line)');
    ylabel('Time variation');
    xlabel('Timestamp index [N/A]');
    %---------------------------------------------------------------------------------
    if Do.ResampleDataToMs
        figure
        plot(list_similar, 'o-')
        grid on;
        title('Repetition of timestamp');
        ylabel('Repetition of timestamp');
        xlabel('Timestamp index [N/A]');
    end
end


% END OF SCRIPT