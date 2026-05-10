//辞書アプリ。「名前：意味」の構造体で管理され、検索、追加、削除が出来る。
//ただし、それはテキストではなくBMP形式の画像ファイルで読込、書込が行われる。
//BMPファイルにすることで、視覚的にどんな内容が入っているのか模様で表せられて面白い。

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct{
    char name[100];
    char meaning[500];
}data;//辞書データ構造体。単語名と意味を格納。

typedef enum{
    MODE_NAME = 0,
    MODE_MEANING = 1
}parseMode;//列挙体。単語の管理を担う状態を敢えて名前(列挙体)で管理

//[基本]指定したピクセルの先頭アドレスを取得する関数
unsigned char* getPixelPtr(unsigned char *image, int index){
    return &image[index * 3];//1ピクセルは3バイトで管理されているので、i*3で各ピクセルの先頭アドレスを取得できる。
}

#define MAX_WORDS 100
#define MAX_FILENAME 60

//========辞書メニュー========
//1 一覧
void showList(data *dict, int index, int count){
    if(index == count){
        printf("----------------------------\n");
        return;
    }
    if(index == 0){
        printf("--- 登録済みデータ一覧 (%d件) ---\n", count);
    }
    printf("【%d】 %s : %s\n",index + 1, dict[index].name, dict[index].meaning);
    showList(dict, index + 1, count);
}

//2 言葉検索
void searchWord(data *dict, int count) {
    char searchName[MAX_WORDS];
    printf("検索する単語を入力: ");
    scanf("%s", searchName);
    
    int found = 0;
    for(int i = 0; i < count; i++){
        if(strcmp(dict[i].name, searchName) == 0){//strcmp:文字列の比較
            printf("【意味】 %s\n", dict[i].meaning);
            found = 1;
            break;
        }
    }
    if(!found) printf("その単語は見つかりませんでした。\n");
}

//3 追加
void addWord(data *dict, int *count) {
    if(*count >= MAX_WORDS){
        printf("辞書がいっぱいです（最大%d件）。\n", MAX_WORDS);
        return;
    }
    printf("新しい単語名: ");
    scanf("%s", dict[*count].name);
    
    //重複チェック
    for(int i = 0; i < *count; i++){
        if(strcmp(dict[i].name, dict[*count].name) == 0){
            printf("その単語は既に存在します。追加を中止します。\n");
            dict[*count].name[0] = '\0';
            return;
        }
    }
    
    printf("その意味: ");
    scanf(" %[^\n]", dict[*count].meaning);
    (*count)++; // カウントを増やす
    printf("単語を追加しました。\n");
}

//4　削除
void deleteWord(data *dict, int *count) {
    int delIndex;
    printf("削除するデータの番号を入力してください: ");
    scanf("%d", &delIndex);

    if(delIndex < 1 || delIndex > *count){
        printf("無効な番号です。\n");
    }else{
        int target = delIndex - 1;                
        for(int i = target; i < *count - 1; i++){
            dict[i] = dict[i+1];
        }
        memset(&dict[*count-1], 0, sizeof(data));
        (*count)--; // カウントを減らす
        printf("削除しました。\n");
    }
}

//5　ソート(クイックソート)
void sortDictionary(data *dict, int count) {
    if(count < 2){
        printf("データが少ないためソートの必要がありません。\n");
        return;
    }
    //クイックソートの実装
    int pivotIndex = count / 2;
    char *pivot = dict[pivotIndex].name;
    int i, j;
    data temp;
    for(i = 0, j = count - 1; ; i++, j--){
        while(strcmp(dict[i].name, pivot) < 0) i++;
        while(strcmp(dict[j].name, pivot) > 0) j--;
        if(i >= j) break;
        temp = dict[i];
        dict[i] = dict[j];
        dict[j] = temp;
    }
    sortDictionary(dict, i);
    sortDictionary(dict + i, count - i);

    printf("UTF-8順にソートしました。\n");
}

//6 辞書情報の確認（ステータス表示）
void showDictStatus(char *filename, int count) {
    printf("\n--- 辞書ステータス ---\n");
    printf("現在のファイル: %s\n", filename);
    printf("登録済みデータ数: %d 件\n", count);
    printf("残り登録可能数: %d 件\n", 100 - count);
    printf("----------------------\n");
}

//===============================================================================
int main(void){
    FILE *fp;
    char filename[MAX_FILENAME];

    //共通で使用する変数
    int width = 50; //デフォルト幅
    int height = 50;//デフォルト高さ
    unsigned char *trueData = NULL;//パディングなしデータ
    unsigned char *tmpData = NULL;//パディングありデータ（ファイル入出力用）
    int dataSize = 0;
    int lineSize = 0;
    
    //BMPヘッダーの初期値（新規作成の場合はこれを使用）
    char header[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
    char info[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0 };

    //辞書データ
    data dictionary[MAX_WORDS];
    int dictIndex = 0;
    memset(dictionary, 0, sizeof(dictionary));

    printf("ようこそ。これはBMPファイルでデータを管理する辞書アプリです。\nそれでは辞書の機能を実行します。\n");
    printf("辞書ファイルの名前を.bmpも含めて入力してください（存在しない場合は新規作成します）。\n");
    scanf("%s", filename);

    //ファイルを開く
    if((fp = fopen(filename, "rb")) != NULL){
        //既存ファイルの読み込み
        fread(header, sizeof(char), 14, fp);//BMPヘッダー読み込み(更新した)
        fread(info, sizeof(char), 40, fp);

        if(header[0] != 'B' || header[1] != 'M'){
            printf("これはBMPファイルではありません。終了します。\n");
            fclose(fp);
            return 1;
        }

        width = *(int *)&info[4];//info[4]は1ビットのデータが保存されているが、そのアドレスをint型ポインタにキャストすると、それの値は後ろ3バイト分をさらに読み込んだint型の値になる。(幅は4-8バイト目に保存されている)
        height = *(int *)&info[8];
        short bitCount = *(short *)&info[14];

        printf("BMPファイルを確認しました。幅: %d、高さ: %d、色深度: %dビット\n", width, height, bitCount);

        if(bitCount != 24){
            printf("24ビットBMP以外には対応していません。終了します。\n");
            fclose(fp);
            return 1;
        }

        printf("データの読込みを開始\n");
        
        //サイズ計算
        lineSize = width * 3;//1行のビット数(24ビットRGBなので1ピクセルに3バイト保存されている)
        if(lineSize % 4 != 0){
            lineSize = lineSize + (4 - (lineSize % 4));//パティング分を加える
        }
        dataSize = lineSize * height;//これはパディング込みのサイズ

        //メモリ確保と読み込み
        tmpData = (unsigned char *)malloc(dataSize);//パディング込みのデータを格納するためのメモリ確保(動的な配列みたいなもの)
        fread(tmpData, sizeof(unsigned char), dataSize, fp);
        fclose(fp); //読み込み完了したので閉じる

        //パディング除去
        trueData = (unsigned char *)malloc(width * 3 * height);//パディング除去後のデータ用メモリ確保。
        for(int y = 0; y < height; y++){
            int srcPos = y * lineSize;
            int dstPos = y * (width * 3);
            memcpy(&trueData[dstPos], &tmpData[srcPos], width * 3);//trueDataにパディング開始までの各行のデータをコピー。
        }

        //辞書データの解析
        int strIndex = 0;
        int pixelCount = width * height;
        //enumもある
        parseMode mode = MODE_NAME;

        for(int i = 0; i < pixelCount; i++){
            unsigned char *p = getPixelPtr(trueData, i);//基準のピクセルアドレス取得
            unsigned char b = p[0];
            unsigned char g = p[1];
            unsigned char r = p[2];

            if(b == 0 && g == 0 && r == 0){//黒ドット：名前と意味の区切り
                if(mode == MODE_NAME){//
                    dictionary[dictIndex].name[strIndex] = '\0';//終端文字を入れる。
                    mode = MODE_MEANING;
                    strIndex = 0;
                }
            }
            else if(b == 255 && g == 255 && r == 255){ //白ドット：各単語の区切り
                if(mode == MODE_MEANING){
                    dictionary[dictIndex].meaning[strIndex] = '\0';
                    dictIndex++;
                    mode = MODE_NAME;
                    strIndex = 0;
                    if(dictIndex >= MAX_WORDS) break;
                }
            }
            else {//文字データ
                char *target;
                if(mode == MODE_NAME){
                    target = dictionary[dictIndex].name;
                } else {
                    target = dictionary[dictIndex].meaning;
                }
                //3バイトを順に文字として追加 (0以外なら)
                if(b != 0) target[strIndex++] = b;//リテラルではなく変数
                if(g != 0) target[strIndex++] = g;
                if(r != 0) target[strIndex++] = r;
            }
        }
        printf("---読み込んだ単語名 (%d件)---\n", dictIndex);
        for(int i = 0; i < dictIndex; i++){
            printf("%s", dictionary[i].name);
            if(i < dictIndex - 1) printf(", ");
            if((i+1) % 5 == 0) printf("\n");
        }
        printf("\n----------------------------\n");

    }else{
        //---新規作成---
        printf("ファイルが見つかりません。新規作成します。\n");
        printf("デフォルトサイズ: %dx%d で作成します。\n", width, height);

        //ヘッダー情報の更新
        memcpy(&info[4], &width, sizeof(int));
        memcpy(&info[8], &height, sizeof(int));

        //サイズ計算
        lineSize = width * 3;
        if(lineSize % 4 != 0) lineSize += (4 - (lineSize % 4));
        dataSize = lineSize * height;

        int fileSize = 54 + dataSize;
        memcpy(&header[2], &fileSize, sizeof(int));

        //メモリ確保と初期化
        tmpData = (unsigned char *)malloc(dataSize);
        memset(tmpData, 0, dataSize);
        
        trueData = (unsigned char *)malloc(width * 3 * height);
        memset(trueData, 0, width * 3 * height);
    }

    //---メインメニュー---
    while(1){
        printf("\n===メニュー===\n");
        printf("1: 一覧表示\n");
        printf("2: 検索\n");
        printf("3: 追加\n");
        printf("4: 削除\n");
        printf("5: ソート\n");
        printf("6: ステータス表示\n");
        printf("7: 保存して終了\n");
        printf("0: 保存せずに終了\n");
        printf("選択してください > ");
        
        int choice;
        scanf("%d", &choice);

        if(choice == 0){
            printf("終了します。\n");
            break;
        }
        else if(choice == 1){
            showList(dictionary, 0, dictIndex);
        }
        else if(choice == 2){
            searchWord(dictionary, dictIndex);
        }
        else if(choice == 3){
            addWord(dictionary, &dictIndex);//アドレスを渡す
        }
        else if(choice == 4){
            deleteWord(dictionary, &dictIndex);//アドレスを渡す
        }
        else if(choice == 5){
            sortDictionary(dictionary, dictIndex);
        }
        else if(choice == 6){
            showDictStatus(filename, dictIndex);
        }
        else if(choice == 7){
            //保存処理
            printf("保存処理を開始します...\n");

            //trueDataをクリア
            memset(trueData, 0, width * 3 * height);

            int pixelIndex = 0;
            for(int i = 0; i < dictIndex; i++){
                //名前
                int len = 0;
                while(dictionary[i].name[len] != '\0') len++;
                for(int k = 0; k < len; k += 3){
                    unsigned char b = dictionary[i].name[k];
                    unsigned char g = (k+1 < len) ? dictionary[i].name[k+1] : 0;
                    unsigned char r = (k+2 < len) ? dictionary[i].name[k+2] : 0;
                    unsigned char *p = getPixelPtr(trueData, pixelIndex);
                    p[0] = b;
                    p[1] = g;
                    p[2] = r;
                    pixelIndex++;
                }
                //区切り（黒）
                unsigned char *p = getPixelPtr(trueData, pixelIndex);
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
                pixelIndex++;

                //意味
                len = 0;
                while(dictionary[i].meaning[len] != '\0') len++;
                for(int k = 0; k < len; k += 3){
                    unsigned char b = dictionary[i].meaning[k];
                    unsigned char g = (k+1 < len) ? dictionary[i].meaning[k+1] : 0;
                    unsigned char r = (k+2 < len) ? dictionary[i].meaning[k+2] : 0;
                    p = getPixelPtr(trueData, pixelIndex);
                    p[0] = b;
                    p[1] = g;
                    p[2] = r;
                    pixelIndex++;
                }
                //区切り（白）
                p = getPixelPtr(trueData, pixelIndex);
                p[0] = 255;
                p[1] = 255;
                p[2] = 255;
                pixelIndex++;

                if(pixelIndex >= width * height){
                    printf("画像サイズ不足のため一部保存されませんでした。\n");
                    break;
                }
            }

            //パディング付加
            memset(tmpData, 0, dataSize);
            for(int y = 0; y < height; y++){
                int srcPos = y * (width * 3);
                int dstPos = y * lineSize;
                memcpy(&tmpData[dstPos], &trueData[srcPos], width * 3);
            }

            // ファイル書き込み
            fp = fopen(filename, "wb");
            if(fp == NULL){
                printf("ファイルの保存に失敗しました。\n");
            } else {
                fwrite(header, sizeof(char), 14, fp);
                fwrite(info, sizeof(char), 40, fp);
                fwrite(tmpData, sizeof(unsigned char), dataSize, fp);
                fclose(fp);
                printf("保存が完了しました。\n");
            }
            break;
        }
    }

    //メモリ解放
    if(trueData) free(trueData);
    if(tmpData) free(tmpData);
    
    return 0;
}