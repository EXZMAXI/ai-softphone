<?xml version="1.0" encoding="utf-8"?>
<!--
  AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
  https://ai-softphone.com

  SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
  SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
  SPDX-License-Identifier: GPL-2.0-or-later

  This file is part of AI Softphone. There is NO WARRANTY, to the extent
  permitted by law. See the file LICENSE for the full terms.
-->
<!DOCTYPE TS>
<TS version="2.1" language="vi" sourcelanguage="en">
<context>
    <name>AnswerPanel</name>
    <message>
        <source>%1…</source>
        <extracomment>A prompt that needs a question typed before it can run. %1 is the prompt&apos;s own name.</extracomment>
        <translation>%1…</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts what a model wrote on the clipboard.</extracomment>
        <translation>Sao chép</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Lưu thành…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Xoá</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Đang làm…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Lưu câu trả lời</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Tệp văn bản (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Mọi tệp (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Xoá cái này?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Hỏi</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Câu hỏi của bạn</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Mô hình trả lời từ cuộc trò chuyện này.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Sao chép câu trả lời này</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Cuộc trò chuyện này đang được gỡ băng.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Sao chép trao đổi với mô hình</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Sao chép dòng này</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Xử lý</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Mô hình không tìm thấy dấu hiệu cảnh báo nào trong cuộc trò chuyện này.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Mô hình không viết gì về cuộc trò chuyện này.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 sẽ bị xoá. Cuộc trò chuyện và bản gỡ băng vẫn được giữ, và bạn có thể yêu cầu lại. Việc xoá không hoàn tác được.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Xử lý</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Xử lý</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Xử lý các cuộc trò chuyện tự động</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Tắt thì không có gì xảy ra cho tới khi bạn yêu cầu trong cửa sổ bản ghi. Bật thì các quy tắc bên dưới chạy — và đó là thứ biến một cuộc trò chuyện thành bản tóm tắt, một hạng mục và mọi thứ khác mà không ai phải nhấn gì. Một mô hình trên đám mây tính phí cho từng việc đó.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Hạn mức tiền, mỗi tháng</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Hạn mức token, mỗi tháng</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Hai hạn mức, trên hai thứ có thể dùng để đếm một tháng. Hễ chạm một trong hai, các quy tắc dừng lại cho tới khi sang tháng. Việc bạn tự yêu cầu thì không bao giờ bị dừng.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Mô hình ngôn ngữ</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Thêm</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Thứ đọc bản gỡ băng và viết về nó. Một mô hình trên máy của bạn giữ mọi cuộc trò chuyện ở trong nhà và chạy không tốn gì.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>mặc định</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>tắt</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Chỉ dẫn</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Những gì được hỏi các mô hình. Mỗi cái đều đi kèm chương trình và mỗi cái đều để bạn sửa — và để bạn đưa về như cũ.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>đã sửa</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>tắt</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Quy tắc</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Những gì tự chạy, theo thứ tự này. Mỗi cái kích hoạt nhiều nhất một lần cho mỗi cuộc trò chuyện.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Đưa lên</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Đưa xuống</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Mặc định</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Đưa các chỉ dẫn và quy tắc về như lúc đi kèm chương trình. Các mô hình ngôn ngữ của bạn được để nguyên.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Khôi phục mặc định</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Chi tiêu</translation>
    </message>
</context>
</TS>
