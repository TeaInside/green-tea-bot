import { SearchIcon } from "@heroicons/react/outline";
import Group from "./Group";

function GroupList() {
    return (
        <div className="w-[350px] bg-white h-screen p-4">
            <div className="relative flex">
                <h1 className="text-[30px]  text-gray-600">Group List</h1>
                <span className="absolute top-0 left-[130px] w-6 h-6 text-center text-white bg-red-500 bg-opacity-60 rounded-full">
                    5
                </span>
            </div>
            <hr className="w-full border-gray-200 mt-5" />

            <div className="relative my-2 rounded-lg">
                <div className="absolute inset-y-0 flex items-center pl-3 pointer-events-none">
                    <SearchIcon className="h-5 w-5 text-gray-500" />
                </div>
                <input
                    className="bg-gray-200 w-full h-10 pl-10 rounded-lg outline-none"
                    type="text"
                    placeholder="Search groups"
                />
            </div>

            <div className="flex flex-col overflow-y-auto h-[83%] whitespace-nowrap">
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group Ra Nggenah" lastMessage="Dwi: jancok" />
                <Group namaGroup="Group Nggenah" lastMessage="Dwi: samlekom" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 1" lastMessage="Dwi: hahaha" />
                <Group namaGroup="Group 2" lastMessage="Ilham: Hallo" />
            </div>
        </div>
    );
}

export default GroupList;
