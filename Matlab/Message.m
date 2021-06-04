%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%Message
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Synopsis
% input :   
%           0/1 % afficher à l'écran
%           0/1 % enregistrer dans le fichier journal
%           0/1 % commencer un nouveau fichier
%           message à enregistrer/afficher
%           'KO' ou 'KO' ou 'UDEF' % case sensitive
%           message de l'erreur s'il y en a eu une
% ouput :

%% Example
% Message(0,1,1,'Importation echouée', 'OK', RunID);
% OR
% Message(1,1,1,'Importation réussie', 'KO', RunID);
% OR
% Message(1,1,1,'Importation peut-etre reussite', 'UDEF', RunID);

%% Purpose
% 'message' est une fonction qui peut être appelée pour enregistrer dans un
% fichier dans le répertoire "Log" situé à l'emplacement du programme
% et/ou afficher à l'écran l'état (succès/échec) de l'éxécution d'une commande

%% Metadata
% Written by    : Nathanaël Esnault
% Verified by   : N/A
% Creation date : 2015-10-12
% Version       : 1.1 (finished on ...)
% Modifications :
% Known bugs    :

function Message(affichage_a_lecran, ecriture_dans_le_fichier, bool_nouveau_fichier, message_a_ecrire, etat, RunID)
% disp(pwd);
% affichage_a_lecran = 1;
% ecriture_dans_le_fichier = 0;
% message_a_ecrire = 'Le calcul';
% etat = 'OK';
% verbose = 1;

% if ~boolean(affichage_a_lecran) ||...
%         ~boolean(ecriture_dans_le_fichier) ||...
%         ~boolean(bool_nouveau_fichier) ||...
%         ~ischar(message_a_ecrire) ||...
%         ~ischar(etat)
%     error('Un des argument d''entrée n''a pas le bon format');
% end

%% Mise en forme du message
formatOut = 'yyyy-mm-dd--HH-MM-SS-FFF';

if (strcmp(etat,'UDEF'))
    Imprime = [datestr(now,formatOut) ' '  message_a_ecrire];
else
    Imprime = [datestr(now,formatOut) ' '  message_a_ecrire ' - state : ' etat];
end


% [ST,I] = dbstack('-completenames')
% ST = 
%     file: 'I:\MATLABFiles\mymfiles\lengthofline.m'
%     name: 'lengthofline'
%     line: 28
% I =
%      1
%% Affichage à l'écran

if affichage_a_lecran == 1
    disp(Imprime);
end

%% Ecriture dans le fichier

if ecriture_dans_le_fichier
    if exist([cd '\Log'],'dir')
        %listing des fichiers et dossiers dans le chemin donné
        MyDirInfo = dir([cd '\Log\*.txt']);
        
        if ~isempty(MyDirInfo)  %repertory exists + file exists
            liste_fichier_log = 'empty';
            
            
            if size(MyDirInfo,1) == 1 && bool_nouveau_fichier == 0%si il n'y a qu'un seul fichier Log ET que continuer est activé%%MODIF
                %on ouvre ce seul fichier
                fid = fopen( [cd '\Log\' MyDirInfo(1).name], 'r' );
                i = 1;
                tline = fgetl(fid);
                Text_from_file{i} = tline;
                while ischar(tline)
                    i = i+1;
                    tline = fgetl(fid);
                    Text_from_file{i} = tline;
                end
                fclose(fid);
                % Change cell Text_from_file
                Text_from_file{i} = Imprime;
                Text_from_file{i+1} = -1;
                % Write cell Text_from_file into txt
                fid = fopen([cd '\Log\' MyDirInfo(1).name], 'w');
                for i = 1:numel(Text_from_file)
                    if Text_from_file{i} == -1
                        fprintf(fid,'%s', Text_from_file{i});
                        break
                    else
                        fprintf(fid,'%s\n', Text_from_file{i});
                    end
                end
                fclose(fid);
                
                
            elseif bool_nouveau_fichier == 0 % s'il y a plusieurs fichiers Log ET continuer est activé
                liste_fichier_log = [];
                
                for cpt_file_or_folder = 1 : size(MyDirInfo,1)
                    if ~isempty(strfind(MyDirInfo(cpt_file_or_folder).name, 'Log-'));
                        liste_fichier_log = [ liste_fichier_log cpt_file_or_folder];
                    end
                end
                %Recherche du fichier Log le plus récent
                
                %Methode par titre de fichier
                for cpt_fichier_log = 1 : size(liste_fichier_log,2)
                    
                    [tocken1, remain1] = strtok(MyDirInfo(liste_fichier_log(cpt_fichier_log)).name,'Log-');
                    [tocken2, ~] = strtok([tocken1, remain1],'.txt');
                    date_creation_fichier_log(cpt_fichier_log) = datenum(tocken2,formatOut);
                    
                end
                
                    [~,indice_fichier_le_plus_recent] = max(date_creation_fichier_log);
                    
                    
                    fid = fopen( [cd '\Log\' MyDirInfo(liste_fichier_log(indice_fichier_le_plus_recent)).name], 'r' );
                    i = 1;
                    tline = fgetl(fid);
                    Text_from_file{i} = tline;
                    while ischar(tline)
                        i = i+1;
                        tline = fgetl(fid);
                        Text_from_file{i} = tline;
                    end
                    fclose(fid);
                    % Change cell Text_from_file
                    Text_from_file{i} = Imprime;
                    Text_from_file{i+1} = -1;
                    % Write cell Text_from_file into txt
                    fid = fopen( [cd '\Log\' MyDirInfo(liste_fichier_log(indice_fichier_le_plus_recent)).name], 'w' );
                    for i = 1:numel(Text_from_file)
                        if Text_from_file{i} == -1
                            fprintf(fid,'%s', Text_from_file{i});
                            break
                        else
                            fprintf(fid,'%s\n', Text_from_file{i});
                        end
                    end
                    fclose(fid);
                 
                %Methode par metadata
            end
            
        end
        
        
        
        %si le dossier Log est vide OU aucun fichier Log trouvé OU
        %continuer est désactivé
        %if isempty(MyDirInfo) || strcmp(liste_fichier_log,'empty') || isempty(liste_fichier_log) || bool_nouveau_fichier == 1
         if isempty(MyDirInfo) || isempty(liste_fichier_log) || bool_nouveau_fichier == 1
            %Création du fichier
            fid = fopen( [cd '\Log\Log-' RunID '.txt'], 'wt' );
            %Remplissage du fichier
            fprintf(fid, '%s\n', 'Log file created thanks to the function : Message (v01r03)');
            fprintf(fid, '%s\n', 'The function Message.m (v01r03) is the intellectual property of Nathanael Esnault');
            fprintf(fid, '%s\n', 'Any action to distribute, share or copy, partially or completely...');
            fprintf(fid, '%s\n', 'this function is liable if done without explicit agreement of the owner of this function');
            fprintf(fid, '%s\n', 'By using this function, you agree to the terms and conditions written...');
            fprintf(fid, '%s\n', 'in the txt file present in the same folder as the function');
            fprintf(fid, '%s\n', '----------------------------------------------------------');
            fprintf(fid, '%s\n', Imprime);
            fclose(fid);
        end
        
    else %repertory NOT exists + file NOT exists
        
        %Création du répertoire
        mkdir([cd '\Log']);
        %Création du fichier
        fid = fopen( [cd '\Log\Log-' RunID '.txt'], 'wt' );
         fprintf(fid, '%s\n', 'Log file created thanks to the function : Message (v01r00)\n Log folder created');
        %Remplissage du fichier
        fprintf(fid, '%s\n', Imprime);
        fclose(fid);
        
    end
end
end