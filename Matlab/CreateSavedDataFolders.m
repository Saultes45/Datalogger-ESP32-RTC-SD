try
    if ~exist([cd '\Data'],'dir')
        mkdir([cd '\Data']);
%         Message(1,1,0,'Data folder created', 'OK', RunID);
    end
    if ~exist([cd '\Data\Test-' RunID],'dir')
        mkdir([cd '\Data\Test-' RunID]);
%         Message(1,1,0,'Test folder, within the Data folder, created', 'OK', RunID);
    end
catch error
    disp(error);
    occured_error = 1;
%     Message(1,1,0,'Saving destination folders couldn''t be created', 'KO', RunID);
end